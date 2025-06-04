#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept> // For std::runtime_error
#include <cstring>   // For memcpy
#include <algorithm> // For std::transform
#include <cctype>    // For ::tolower

// OpenSSL headers
#include <openssl/evp.h>
#include <openssl/rand.h> 
#include <openssl/err.h>  
#include <openssl/conf.h> 

// --- Configuration ---
const int AES_KEY_BITS = 256;
const int AES_KEY_BYTES = AES_KEY_BITS / 8;
const int AES_IV_BYTES = 16;    
const int AES_BLOCK_BYTES = 16; 
const int PBKDF2_ITERATIONS = 10000;

// --- OpenSSL Error Handling ---
// This function is CRUCIAL. If OpenSSL operations fail, it prints why.
void handle_openssl_errors(const std::string& context_message = "") {
    unsigned long err_code;
    std::string error_string = context_message;
    bool errors_found = false;
    while ((err_code = ERR_get_error())) {
        errors_found = true;
        error_string += "\nOpenSSL Error: "; // Added newline for better formatting
        char err_buf[256];
        ERR_error_string_n(err_code, err_buf, sizeof(err_buf));
        error_string += err_buf;
    }
    if (errors_found) {
        // Print to cerr and throw, or print and exit(1)
        // Throwing allows a calling C++ context to catch if needed,
        // but for a standalone tool, printing and exiting is also common.
        // We'll keep throwing as it integrates with the main try-catch.
        throw std::runtime_error(error_string);
    }
}

// --- Key/IV Derivation ---
bool derive_key_and_iv(const std::string& passphrase, const unsigned char* salt, int salt_len,
                       unsigned char* key_out, int key_out_len,
                       unsigned char* iv_out, int iv_out_len) {
    if (key_out_len != AES_KEY_BYTES || iv_out_len != AES_IV_BYTES) {
        // This is a programming error check, should be minimal.
        // std::cerr << "Error: Key/IV length config error." << std::endl; 
        return false; 
    }

    if (PKCS5_PBKDF2_HMAC(passphrase.c_str(), passphrase.length(),
                          salt, salt_len,
                          PBKDF2_ITERATIONS, EVP_sha256(),
                          key_out_len, key_out) != 1) {
        handle_openssl_errors("Key derivation (PKCS5_PBKDF2_HMAC for key) failed.");
        return false; // Should not be reached if handle_openssl_errors throws
    }

    std::vector<unsigned char> iv_salt(salt, salt + salt_len);
    iv_salt.push_back('I'); 
    iv_salt.push_back('V');

    if (PKCS5_PBKDF2_HMAC(passphrase.c_str(), passphrase.length(),
                          iv_salt.data(), iv_salt.size(),
                          PBKDF2_ITERATIONS, EVP_sha256(),      
                          iv_out_len, iv_out) != 1) {
        handle_openssl_errors("IV derivation (PKCS5_PBKDF2_HMAC for IV) failed.");
        return false; // Should not be reached
    }
    return true;
}

// --- AES Operation (Always Padded) ---
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
            throw std::runtime_error("Unsupported AES mode: " + mode); // Basic validation
        }

        if (!(ctx = EVP_CIPHER_CTX_new())) {
            handle_openssl_errors("EVP_CIPHER_CTX_new failed.");
            return false; // Should not be reached
        }

        bool is_encrypt = (operation == "encrypt");
        const unsigned char* current_iv = (mode == "ECB" ? NULL : iv);

        if (1 != EVP_CipherInit_ex(ctx, cipher_type, NULL, key, current_iv, (is_encrypt ? 1 : 0))) {
             handle_openssl_errors(std::string(is_encrypt ? "EVP_EncryptInit_ex" : "EVP_DecryptInit_ex") + " failed.");
             EVP_CIPHER_CTX_free(ctx); return false; // Should not be reached
        }

        if (1 != EVP_CIPHER_CTX_set_padding(ctx, 1)) { // ALWAYS ENABLE PADDING
             handle_openssl_errors("EVP_CIPHER_CTX_set_padding (to enable) failed.");
             EVP_CIPHER_CTX_free(ctx); return false; // Should not be reached
        }

        if (input_len > 0) {
            if (1 != EVP_CipherUpdate(ctx, output_data, &len, input_data, static_cast<int>(input_len))) {
                handle_openssl_errors("EVP_CipherUpdate failed.");
                EVP_CIPHER_CTX_free(ctx); return false; // Should not be reached
            }
            output_len = len;
        } else {
            output_len = 0; 
        }

        int final_len = 0;
        if (1 != EVP_CipherFinal_ex(ctx, output_data + output_len, &final_len)) {
            // Simplified: No extra "Warning" print here. 
            // If this fails (e.g. padding error on decrypt), handle_openssl_errors will report it.
            handle_openssl_errors(std::string(is_encrypt ? "EVP_EncryptFinal_ex" : "EVP_DecryptFinal_ex") + " failed.");
            EVP_CIPHER_CTX_free(ctx); return false; // Should not be reached
        }
        output_len += final_len;

    } catch (...) { // Catch any exception from within try, including what handle_openssl_errors throws
        if(ctx) EVP_CIPHER_CTX_free(ctx);
        throw; // Re-throw to be caught by main or terminate
    }
    
    if(ctx) EVP_CIPHER_CTX_free(ctx);
    return true;
}

// --- File Handling Utilities ---
std::vector<unsigned char> read_file_bytes(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open input file: " + file_path);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(static_cast<size_t>(size)); 
    if (size > 0) { 
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            throw std::runtime_error("Error: Could not read input file: " + file_path);
        }
    }
    file.close();
    return buffer;
}

void write_file_bytes(const std::string& file_path, const std::vector<unsigned char>& buffer) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open output file for writing: " + file_path);
    }
    if (!buffer.empty()) { 
        file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        if (!file.good()) {
            throw std::runtime_error("Error: Could not write to output file: " + file_path);
        }
    }
    file.close();
}

// --- Main Application Logic ---
int main(int argc, char* argv[]) {
    if (argc != 6) { 
        std::cerr << "Usage: " << argv[0] << " <input_path> <aes_passphrase> <output_path> <encrypt|decrypt> <ECB|CBC>" << std::endl;
        return 1;
    }

    std::string input_path = argv[1];
    std::string passphrase = argv[2];
    std::string output_path = argv[3];
    std::string operation_str = argv[4];
    std::string mode_str = argv[5];

    if (operation_str != "encrypt" && operation_str != "decrypt") {
        std::cerr << "Error: Invalid operation. Must be 'encrypt' or 'decrypt'." << std::endl; return 1;
    }
    if (mode_str != "ECB" && mode_str != "CBC") {
        std::cerr << "Error: Invalid mode. Must be 'ECB' or 'CBC'." << std::endl; return 1;
    }

    // Minimal logging
    // std::cout << "Processing: " << operation_str << " " << input_path << " -> " << output_path << " (Mode: " << mode_str << ")" << std::endl;

    try {
        OpenSSL_add_all_algorithms(); 
        ERR_load_crypto_strings(); // Load error strings for libcrypto

        std::vector<unsigned char> data_to_process = read_file_bytes(input_path);
        
        unsigned char fixed_salt[] = "UltraSimpleSalt!"; // Changed salt slightly for distinctness
        unsigned char derived_key[AES_KEY_BYTES];
        unsigned char derived_iv[AES_IV_BYTES];

        if (!derive_key_and_iv(passphrase, fixed_salt, sizeof(fixed_salt) - 1, derived_key, AES_KEY_BYTES, derived_iv, AES_IV_BYTES)) {
            // derive_key_and_iv now calls handle_openssl_errors which throws,
            // or returns false for config error (which shouldn't happen with fixed AES_KEY_BYTES/AES_IV_BYTES).
            // So, this path might not be strictly needed if derive_key_and_iv always throws on failure.
            // For safety, keeping a generic error.
            std::cerr << "Error: Failed to derive AES key and IV." << std::endl;
            return 1;
        }
        
        std::vector<unsigned char> processed_data; 
        processed_data.resize(data_to_process.size() + AES_BLOCK_BYTES); 
        int actual_output_len = 0;
        
        if (!aes_openssl_operation(data_to_process.data(), data_to_process.size(),
                                   processed_data.data(), actual_output_len,
                                   derived_key, derived_iv,
                                   operation_str, mode_str)) {
            // aes_openssl_operation now calls handle_openssl_errors which throws.
            // This path might not be strictly needed.
            std::cerr << "Error: AES processing failed." << std::endl;
            // ERR_print_errors_fp(stderr); // Already handled by handle_openssl_errors
            return 1; 
        }
        processed_data.resize(actual_output_len); 

        write_file_bytes(output_path, processed_data);
        // std::cout << operation_str << " completed successfully." << std::endl;

    } catch (const std::exception& e) {
        // This will catch errors thrown by handle_openssl_errors, read_file_bytes, write_file_bytes, etc.
        std::cerr << "An error occurred: " << e.what() << std::endl;
        // Ensure OpenSSL error strings are available for e.what() if it's from handle_openssl_errors
        // ERR_free_strings(); // Call at the very end if not relying on auto-cleanup
        return 1;
    }

    ERR_free_strings(); // Clean up loaded error strings
    // EVP_cleanup(); // Deprecated, OpenSSL_cleanup() is newer but auto-cleanup is common
    return 0;
}
