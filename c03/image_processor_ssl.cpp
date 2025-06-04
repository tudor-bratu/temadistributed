#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept> // For std::runtime_error
#include <cstring>   // For memcpy, memset
#include <omp.h>     // OpenMP library

// OpenSSL headers
#include <openssl/evp.h>
#include <openssl/rand.h> // For RAND_bytes if generating random salt/IV
#include <openssl/err.h>  // For error reporting
#include <openssl/conf.h> // For EVP_cleanup, OpenSSL_add_all_algorithms

// --- Configuration ---
const int BMP_HEADER_SIZE = 54; // Common size for BMP header
const int PIXEL_DATA_OFFSET_LOCATION = 10; // Location of pixel data offset in BMP header
const int AES_KEY_BITS = 256; // Using AES-256
const int AES_KEY_BYTES = AES_KEY_BITS / 8;
const int AES_IV_BYTES = 16;    // AES block size is 128 bits (16 bytes), so IV is 16 bytes
const int AES_BLOCK_BYTES = 16; // AES block size
const int PBKDF2_ITERATIONS = 10000; // Iterations for PBKDF2

// --- OpenSSL Error Handling ---
void handle_openssl_errors(const std::string& context_message = "") {
    unsigned long err_code;
    std::string error_string = context_message;
    while ((err_code = ERR_get_error())) {
        error_string += "OpenSSL Error: ";
        error_string += ERR_error_string(err_code, NULL);
        error_string += "\n";
    }
    if (!error_string.empty() && error_string != context_message) { // Check if errors were actually added
        throw std::runtime_error(error_string);
    }
}

// --- Key/IV Derivation ---
// IMPORTANT: In a real application, for encryption, salt should be randomly generated
// and stored/transmitted with the ciphertext. For decryption, the same salt must be used.
// This example uses a FIXED salt for simplicity. DO NOT DO THIS IN PRODUCTION.
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

    // Derive the IV. For simplicity, we derive it like the key but with a different "context"
    // (e.g. by using a different salt or appending something to the passphrase internally).
    // A common good practice for CBC is to generate a cryptographically random IV for each encryption.
    // Here, we deterministically derive it from the passphrase + salt for consistency in this example.
    // To make the IV derivation different from key derivation with the same inputs,
    // we can use a different digest or a slightly modified salt concept for the IV part.
    // For this example, we'll use a simple approach: use the derived key to seed an IV derivation,
    // or use PBKDF2 again with a different "purpose" (e.g. different salt or different digest).
    // Let's use PBKDF2 again but with a different digest (e.g., MD5) for the IV part to ensure it's different.
    // This is just for example; random IV generation is preferred for CBC encryption.
    if (PKCS5_PBKDF2_HMAC(passphrase.c_str(), passphrase.length(), // Passphrase
                          salt, salt_len,                           // Salt (could use a modified salt for IV)
                          PBKDF2_ITERATIONS / 2,                  // Fewer iterations or different count
                          EVP_md5(),                              // Different hash for IV
                          iv_out_len, iv_out) != 1) {
        std::cerr << "Error: PKCS5_PBKDF2_HMAC failed for IV derivation." << std::endl;
        handle_openssl_errors("IV derivation failed: ");
        return false;
    }
    return true;
}

// --- AES Operation using OpenSSL EVP Interface ---
bool aes_openssl_operation(
    const unsigned char* input_data, int input_len,
    unsigned char* output_data, int& output_len, // output_data buffer must be large enough
    const unsigned char* key,
    const unsigned char* iv,
    const std::string& operation, // "encrypt" or "decrypt"
    const std::string& mode,      // "ECB" or "CBC"
    bool enable_padding           // PKCS#7 padding
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

        if (is_encrypt) {
            if (1 != EVP_EncryptInit_ex(ctx, cipher_type, NULL, key, iv)) {
                handle_openssl_errors("EVP_EncryptInit_ex failed: ");
                EVP_CIPHER_CTX_free(ctx); return false;
            }
        } else { // Decrypt
            if (1 != EVP_DecryptInit_ex(ctx, cipher_type, NULL, key, iv)) {
                handle_openssl_errors("EVP_DecryptInit_ex failed: ");
                EVP_CIPHER_CTX_free(ctx); return false;
            }
        }

        // Set padding
        if (1 != EVP_CIPHER_CTX_set_padding(ctx, enable_padding ? 1 : 0)) {
             handle_openssl_errors("EVP_CIPHER_CTX_set_padding failed: ");
             EVP_CIPHER_CTX_free(ctx); return false;
        }


        if (1 != EVP_CipherUpdate(ctx, output_data, &len, input_data, input_len)) {
            handle_openssl_errors("EVP_CipherUpdate failed: ");
            EVP_CIPHER_CTX_free(ctx); return false;
        }
        output_len = len;

        if (is_encrypt) {
            if (1 != EVP_EncryptFinal_ex(ctx, output_data + len, &len)) {
                handle_openssl_errors("EVP_EncryptFinal_ex failed: ");
                EVP_CIPHER_CTX_free(ctx); return false;
            }
        } else { // Decrypt
            if (1 != EVP_DecryptFinal_ex(ctx, output_data + len, &len)) {
                // This can fail if key/IV is wrong, ciphertext is corrupt, or padding is incorrect
                std::cerr << "Warning: EVP_DecryptFinal_ex failed. This often means incorrect key/IV, corrupted data, or padding error." << std::endl;
                handle_openssl_errors("EVP_DecryptFinal_ex failed (check key/IV/data/padding): ");
                // output_len will remain as whatever EVP_CipherUpdate produced, final block fails.
                // For robust error handling, you might want to clear output_data or signal a more specific error.
                EVP_CIPHER_CTX_free(ctx); return false; // Indicate failure
            }
        }
        output_len += len;

    } catch (const std::exception& e) {
        std::cerr << "Exception in aes_openssl_operation: " << e.what() << std::endl;
        if(ctx) EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    if(ctx) EVP_CIPHER_CTX_free(ctx);
    return true;
}


// --- BMP File Handling Utilities (from previous version) ---
std::vector<unsigned char> read_file_bytes(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open file for reading: " + file_path);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Error: Could not read file: " + file_path);
    }
    file.close();
    return buffer;
}

void write_file_bytes(const std::string& file_path, const std::vector<unsigned char>& buffer) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open file for writing: " + file_path);
    }
    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    if (!file.good()) {
        throw std::runtime_error("Error: Could not write to file: " + file_path);
    }
    file.close();
}

uint32_t get_pixel_data_offset(const std::vector<unsigned char>& header_data_segment) {
    // Use the provided segment, assuming it's at least PIXEL_DATA_OFFSET_LOCATION + 4 bytes long
    if (header_data_segment.size() < PIXEL_DATA_OFFSET_LOCATION + 4) {
         // This might happen if full_image_data.size() < BMP_HEADER_SIZE initially.
         // The main function checks full_image_data.size() >= BMP_HEADER_SIZE.
         // Then bmp_header is created with BMP_HEADER_SIZE. So this path should be safe.
        throw std::runtime_error("Error: BMP header segment is too small to read pixel data offset.");
    }
    return static_cast<uint32_t>(header_data_segment[PIXEL_DATA_OFFSET_LOCATION]) |
           static_cast<uint32_t>(header_data_segment[PIXEL_DATA_OFFSET_LOCATION + 1]) << 8 |
           static_cast<uint32_t>(header_data_segment[PIXEL_DATA_OFFSET_LOCATION + 2]) << 16 |
           static_cast<uint32_t>(header_data_segment[PIXEL_DATA_OFFSET_LOCATION + 3]) << 24;
}


// --- Main Application Logic ---
int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <input_bmp_path> <aes_passphrase> <output_bmp_path> <encrypt|decrypt> <ECB|CBC>" << std::endl;
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

    // Initialize OpenSSL (recommended for some versions/setups)
    OpenSSL_add_all_algorithms(); // Deprecated in OpenSSL 3.0, but good for compatibility
    ERR_load_crypto_strings();
    // EVP_cleanup(); // Also deprecated, OpenSSL_cleanup() is newer but auto-cleanup is common

    std::cout << "Starting image processing with OpenSSL..." << std::endl;
    std::cout << "Input: " << input_path << ", Output: " << output_path << std::endl;
    std::cout << "Operation: " << operation_str << ", Mode: " << mode_str << std::endl;

    try {
        std::vector<unsigned char> full_image_data = read_file_bytes(input_path);
        if (full_image_data.size() < BMP_HEADER_SIZE) {
            throw std::runtime_error("Error: Input file is smaller than a typical BMP header.");
        }

        std::vector<unsigned char> bmp_header_for_offset_calc(full_image_data.begin(), full_image_data.begin() + BMP_HEADER_SIZE);
        uint32_t pixel_offset = get_pixel_data_offset(bmp_header_for_offset_calc);

        if (pixel_offset >= full_image_data.size() || pixel_offset < BMP_HEADER_SIZE) {
             // Basic sanity check for pixel_offset. It should be at least BMP_HEADER_SIZE
             // if the initial header part was indeed that size.
             // A more robust BMP parser would validate various header fields.
            throw std::runtime_error("Error: Invalid pixel data offset found in BMP header or header too small.");
        }
        
        std::vector<unsigned char> actual_header_data(full_image_data.begin(), full_image_data.begin() + pixel_offset);
        std::vector<unsigned char> pixel_data(full_image_data.begin() + pixel_offset, full_image_data.end());

        if (pixel_data.empty() && operation_str == "encrypt") { // Allow empty pixel data for decryption attempt if header is present
            throw std::runtime_error("Error: No pixel data found in BMP file for encryption.");
        }
        std::cout << "Actual BMP Header size (from offset): " << actual_header_data.size() << " bytes." << std::endl;
        std::cout << "Pixel data size: " << pixel_data.size() << " bytes." << std::endl;

        // --- Derive Key and IV ---
        // IMPORTANT: FIXED SALT - NOT FOR PRODUCTION! Generate & store random salt.
        unsigned char fixed_salt[] = "OpenMP_AES_Salt"; // Example fixed salt
        unsigned char derived_key[AES_KEY_BYTES];
        unsigned char derived_iv[AES_IV_BYTES];

        if (!derive_key_and_iv(passphrase, fixed_salt, sizeof(fixed_salt) - 1, derived_key, AES_KEY_BYTES, derived_iv, AES_IV_BYTES)) {
            throw std::runtime_error("Failed to derive AES key and IV from passphrase.");
        }
        std::cout << "AES Key and IV derived successfully." << std::endl;


        // --- Perform AES operation ---
        std::vector<unsigned char> processed_pixel_data; // To store output

        if (mode_str == "ECB") {
            std::cout << "Processing ECB mode with OpenMP..." << std::endl;
            std::cout << "Number of available OpenMP threads: " << omp_get_max_threads() << std::endl;
            
            // For ECB, we can try to parallelize. However, padding is an issue.
            // If we disable padding, pixel_data size must be a multiple of AES_BLOCK_BYTES.
            // If we enable padding, it should be applied ONCE to the whole data, not per chunk.
            // This example will process ECB chunks in parallel *without padding* per chunk.
            // This means if pixel_data.size() is not a multiple of AES_BLOCK_BYTES,
            // the last partial block will NOT be processed by this loop.
            // A robust solution would pad pixel_data first or handle the remainder.

            if (pixel_data.size() % AES_BLOCK_BYTES != 0) {
                std::cout << "Warning: Pixel data size (" << pixel_data.size()
                          << ") is not a multiple of AES block size (" << AES_BLOCK_BYTES
                          << "). For parallel ECB without padding per chunk, the last partial block will be ignored." << std::endl;
            }
            
            size_t num_full_blocks = pixel_data.size() / AES_BLOCK_BYTES;
            processed_pixel_data.resize(num_full_blocks * AES_BLOCK_BYTES); // Output will be same size as input processed

            bool ecb_parallel_success = true;
            #pragma omp parallel for schedule(static)
            for (size_t block_idx = 0; block_idx < num_full_blocks; ++block_idx) {
                if (!ecb_parallel_success) continue; // Skip if an error occurred in another thread

                const unsigned char* chunk_input_ptr = pixel_data.data() + (block_idx * AES_BLOCK_BYTES);
                unsigned char* chunk_output_ptr = processed_pixel_data.data() + (block_idx * AES_BLOCK_BYTES);
                int chunk_output_len = 0;

                // Process one AES block, NO PADDING for these individual blocks in parallel ECB
                if (!aes_openssl_operation(chunk_input_ptr, AES_BLOCK_BYTES,
                                           chunk_output_ptr, chunk_output_len,
                                           derived_key, derived_iv, // IV is ignored by EVP_aes_ecb anyway
                                           operation_str, mode_str,
                                           false /* no padding for individual blocks */)) {
                    #pragma omp critical
                    {
                        std::cerr << "Error processing ECB block " << block_idx << " in thread " << omp_get_thread_num() << std::endl;
                        ecb_parallel_success = false;
                    }
                } else if (chunk_output_len != AES_BLOCK_BYTES) {
                     #pragma omp critical
                    {
                        std::cerr << "Error: ECB block " << block_idx << " output length mismatch (" << chunk_output_len << ")" << std::endl;
                        ecb_parallel_success = false;
                    }
                }
            } // end omp parallel for

            if (!ecb_parallel_success) {
                throw std::runtime_error("Error occurred during parallel ECB processing.");
            }
            // Handle any remaining partial block if necessary (not done in this simplified loop)

        } else if (mode_str == "CBC") {
            std::cout << "Processing CBC mode serially (OpenMP not used for CBC crypto part due to sequential nature)..." << std::endl;
            // For CBC, process the entire pixel data at once to maintain the chain.
            // Output buffer needs to accommodate potential padding.
            processed_pixel_data.resize(pixel_data.size() + AES_BLOCK_BYTES); // Max possible size with padding
            int actual_output_len = 0;

            if (!aes_openssl_operation(pixel_data.data(), pixel_data.size(),
                                       processed_pixel_data.data(), actual_output_len,
                                       derived_key, derived_iv,
                                       operation_str, mode_str,
                                       true /* enable padding for whole data */)) {
                throw std::runtime_error("Error during serial CBC processing.");
            }
            processed_pixel_data.resize(actual_output_len); // Trim to actual size
        }

        std::cout << "AES processing complete. Processed pixel data size: " << processed_pixel_data.size() << " bytes." << std::endl;

        // Combine header and processed pixel data
        std::vector<unsigned char> output_image_data = actual_header_data;
        output_image_data.insert(output_image_data.end(), processed_pixel_data.begin(), processed_pixel_data.end());

        write_file_bytes(output_path, output_image_data);
        std::cout << "Image processing finished successfully. Output saved to: " << output_path << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        // Clean up OpenSSL
        ERR_free_strings();
        EVP_cleanup(); // Or OpenSSL_cleanup() for newer OpenSSL
        return 1;
    }

    // Clean up OpenSSL
    ERR_free_strings();
    EVP_cleanup(); // Or OpenSSL_cleanup() for newer OpenSSL
    return 0;
}

