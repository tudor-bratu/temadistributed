#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept> // For std::runtime_error
#include <cstring>   // For memcpy, memset
#include <algorithm> // For std::transform
#include <cctype>    // For ::tolower

// OpenSSL headers
#include <openssl/evp.h>
#include <openssl/rand.h> // For RAND_bytes if generating random salt/IV
#include <openssl/err.h>  // For error reporting
#include <openssl/conf.h> // For EVP_cleanup, OpenSSL_add_all_algorithms

// --- Configuration ---
const int AES_KEY_BITS = 256; // Using AES-256
const int AES_KEY_BYTES = AES_KEY_BITS / 8;
const int AES_IV_BYTES = 16;    // AES block size is 128 bits (16 bytes), so IV is 16 bytes
const int AES_BLOCK_BYTES = 16; // AES block size (padding will be to this boundary)
const int PBKDF2_ITERATIONS = 10000; // Iterations for PBKDF2

// --- OpenSSL Error Handling ---
void handle_openssl_errors(const std::string& context_message = "") {
    unsigned long err_code;
    std::string error_string = context_message;
    bool errors_found = false;
    while ((err_code = ERR_get_error())) {
        errors_found = true;
        error_string += "OpenSSL Error: ";
        char buf[256];
        ERR_error_string_n(err_code, buf, sizeof(buf));
        error_string += buf;
        error_string += "\n";
    }
    if (errors_found) {
        throw std::runtime_error(error_string);
    }
}

// --- Key/IV Derivation ---
bool derive_key_and_iv(const std::string& passphrase, const unsigned char* salt, int salt_len,
                       unsigned char* key_out, int key_out_len,
                       unsigned char* iv_out, int iv_out_len) {
    if (key_out_len != AES_KEY_BYTES || iv_out_len != AES_IV_BYTES) {
        std::cerr << "Error: Requested key/IV length mismatch with AES configuration." << std::endl;
        return false;
    }

    // Derive the key
    if (PKCS5_PBKDF2_HMAC(passphrase.c_str(), passphrase.length(),
                          salt, salt_len,
                          PBKDF2_ITERATIONS,
                          EVP_sha256(), // Use SHA-256 for KDF
                          key_out_len, key_out) != 1) {
        std::cerr << "Error: PKCS5_PBKDF2_HMAC failed for key derivation." << std::endl;
        handle_openssl_errors("Key derivation failed: ");
        return false;
    }

    // Derive the IV.
    std::vector<unsigned char> iv_salt(salt, salt + salt_len);
    iv_salt.push_back('I'); // Make IV salt slightly different
    iv_salt.push_back('V');

    if (PKCS5_PBKDF2_HMAC(passphrase.c_str(), passphrase.length(),
                          iv_salt.data(), iv_salt.size(),
                          PBKDF2_ITERATIONS, 
                          EVP_sha256(),      
                          iv_out_len, iv_out) != 1) {
        std::cerr << "Error: PKCS5_PBKDF2_HMAC failed for IV derivation." << std::endl;
        handle_openssl_errors("IV derivation failed: ");
        return false;
    }
    return true;
}

// --- AES Operation using OpenSSL EVP Interface ---
// Padding is NOW ALWAYS ENABLED for each operation.
bool aes_openssl_operation(
    const unsigned char* input_data, size_t input_len, 
    unsigned char* output_data, int& output_len, 
    const unsigned char* key,
    const unsigned char* iv,
    const std::string& operation, 
    const std::string& mode
) {
    EVP_CIPHER_CTX *ctx = NULL;
    const EVP_CIPHER *cipher_type = NULL;
    int len = 0;
    output_len = 0; 

    try {
        if (mode == "ECB") {
            cipher_type = EVP_aes_256_ecb();
        } else if (mode == "CBC") {
            cipher_type = EVP_aes_256_cbc();
        } else {
            throw std::runtime_error("Unsupported AES mode: " + mode);
        }

        if (!(ctx = EVP_CIPHER_CTX_new())) {
            handle_openssl_errors("EVP_CIPHER_CTX_new failed: ");
            return false;
        }

        bool is_encrypt = (operation == "encrypt");
        const unsigned char* current_iv = (mode == "ECB" ? NULL : iv);

        if (1 != EVP_CipherInit_ex(ctx, cipher_type, NULL, key, current_iv, (is_encrypt ? 1 : 0))) {
             handle_openssl_errors(std::string(is_encrypt ? "EVP_EncryptInit_ex" : "EVP_DecryptInit_ex") + " failed: ");
             EVP_CIPHER_CTX_free(ctx); return false;
        }

        // Padding is ALWAYS ENABLED (PKCS#7 padding)
        if (1 != EVP_CIPHER_CTX_set_padding(ctx, 1)) { // 1 enables padding
             handle_openssl_errors("EVP_CIPHER_CTX_set_padding (to enable) failed: ");
             EVP_CIPHER_CTX_free(ctx); return false;
        }

        // Process data. Even if input_len is 0, padding will be added by EncryptFinal_ex.
        if (input_len > 0) {
            if (1 != EVP_CipherUpdate(ctx, output_data, &len, input_data, static_cast<int>(input_len))) {
                handle_openssl_errors("EVP_CipherUpdate failed: ");
                EVP_CIPHER_CTX_free(ctx); return false;
            }
            output_len = len;
        } else {
            output_len = 0; 
        }

        // Finalize. This is where padding is added (encryption) or checked/removed (decryption).
        int final_len = 0;
        if (1 != EVP_CipherFinal_ex(ctx, output_data + output_len, &final_len)) {
            // This can fail if key/IV is wrong, ciphertext is corrupt, or padding is incorrect/missing during decryption.
            std::cerr << "Warning: EVP_CipherFinal_ex failed. Possible reasons: incorrect key/IV, corrupted data, or padding error (e.g. padding missing/malformed during decryption)." << std::endl;
            handle_openssl_errors(std::string(is_encrypt ? "EVP_EncryptFinal_ex" : "EVP_DecryptFinal_ex") + " failed: ");
            EVP_CIPHER_CTX_free(ctx); return false; 
        }
        output_len += final_len;

    } catch (const std::exception& e) {
        std::cerr << "Exception in aes_openssl_operation: " << e.what() << std::endl;
        if(ctx) EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    if(ctx) EVP_CIPHER_CTX_free(ctx);
    return true;
}


// --- File Handling Utilities ---
std::vector<unsigned char> read_file_bytes(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open file for reading: " + file_path);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(static_cast<size_t>(size)); 
    if (size > 0) { 
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            throw std::runtime_error("Error: Could not read file: " + file_path);
        }
    }
    file.close();
    return buffer;
}

void write_file_bytes(const std::string& file_path, const std::vector<unsigned char>& buffer) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open file for writing: " + file_path);
    }
    if (!buffer.empty()) { 
        file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        if (!file.good()) {
            throw std::runtime_error("Error: Could not write to file: " + file_path);
        }
    }
    file.close();
}

// Helper to parse boolean command line arguments (REMOVED as arguments are removed)
// bool parse_bool_arg(const std::string& arg_str, const std::string& arg_name) { ... }


// --- Main Application Logic ---
int main(int argc, char* argv[]) {
    // Expecting 5 arguments + program name = 6
    if (argc != 6) { 
        std::cerr << "Usage: " << argv[0] << " <input_path> <aes_passphrase> <output_path> <encrypt|decrypt> <ECB|CBC>" << std::endl;
        return 1;
    }

    std::string input_path = argv[1];
    std::string passphrase = argv[2];
    std::string output_path = argv[3];
    std::string operation_str = argv[4];
    std::string mode_str = argv[5];
    // is_first_chunk and is_last_chunk arguments and their parsing are removed.

    if (operation_str != "encrypt" && operation_str != "decrypt") {
        std::cerr << "Error: Invalid operation. Must be 'encrypt' or 'decrypt'." << std::endl; return 1;
    }
    if (mode_str != "ECB" && mode_str != "CBC") {
        std::cerr << "Error: Invalid mode. Must be 'ECB' or 'CBC'." << std::endl; return 1;
    }

    OpenSSL_add_all_algorithms(); 
    ERR_load_crypto_strings();

    std::cout << "Starting chunk processing with OpenSSL..." << std::endl;
    std::cout << "Input: " << input_path << ", Output: " << output_path << std::endl;
    std::cout << "Operation: " << operation_str << ", Mode: " << mode_str << std::endl;
    std::cout << "Padding: ALWAYS ENABLED for this operation." << std::endl;


    try {
        std::vector<unsigned char> data_to_process = read_file_bytes(input_path);
        
        std::cout << "Data to process size: " << data_to_process.size() << " bytes." << std::endl;
        
        // --- Derive Key and IV ---
        unsigned char fixed_salt[] = "OpenMP_AES_Salt"; 
        unsigned char derived_key[AES_KEY_BYTES];
        unsigned char derived_iv[AES_IV_BYTES];

        if (!derive_key_and_iv(passphrase, fixed_salt, sizeof(fixed_salt) - 1, derived_key, AES_KEY_BYTES, derived_iv, AES_IV_BYTES)) {
            throw std::runtime_error("Failed to derive AES key and IV from passphrase.");
        }
        std::cout << "AES Key and IV derived successfully." << std::endl;

        // --- Perform AES operation ---
        std::vector<unsigned char> processed_data; 
        processed_data.resize(data_to_process.size() + AES_BLOCK_BYTES); 
        int actual_output_len = 0;
        
        if (!aes_openssl_operation(data_to_process.data(), data_to_process.size(),
                                   processed_data.data(), actual_output_len,
                                   derived_key, derived_iv,
                                   operation_str, mode_str)) {
            throw std::runtime_error("Error during AES processing of the chunk.");
        }
        processed_data.resize(actual_output_len); 

        std::cout << "AES processing complete for this chunk. Processed data size: " << processed_data.size() << " bytes." << std::endl;

        write_file_bytes(output_path, processed_data);
        std::cout << "Chunk processing finished successfully. Output saved to: " << output_path << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        ERR_free_strings();
        return 1;
    }

    ERR_free_strings();
    return 0;
}
