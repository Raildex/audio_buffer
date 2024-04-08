#ifndef INCLUDE_AUDIO_BUFFER_AUDIO_BUFFER_H
#define INCLUDE_AUDIO_BUFFER_AUDIO_BUFFER_H
#include <stddef.h> // for size_t
#ifdef __cplusplus
extern "C" {
#endif
struct rdx_audio_buffer;
typedef struct rdx_audio_buffer rdx_audio_buffer;

typedef enum rdx_audio_buffer_status {
    AB_OK = 0,
    AB_LAST_FRAME,
    AB_UKNWN_ERR,
    AB_ERR_FILE_ACCESS,
    AB_ERR_UNKNWN_FILE_FMT,
    AB_ERR_NO_MEM,

} rdx_audio_buffer_status;

typedef enum rdx_sample_format {
    S8,
    U8,
    S16,
    U16,
    S32,
    U32,
    F32,
    UNSUPPORTED
}rdx_sample_format;

/// @brief Creates an audio buffer object with the given number of bytes. Allocates three buffers for rotation
/// @param buffer_size_in_bytes the amount of bytes used for decoded pcm data
/// @param alloc allocation function for dynamically allocated resources
/// @param dealloc deallocation function dynamically allocated resources
/// @return 
rdx_audio_buffer* rdx_create_audio_stream(size_t buffer_size_in_bytes, void* (*alloc)(size_t),void (*dealloc)(void*));
/// @brief Opens an audio file and writes sample data into buffers
/// @param strm the audio buffer object
/// @param file the file to open
/// @param out_data output pointer to buffer data. @see rdx_fill_next_buffer
/// @param out_data_size output size written. @see rdx_fill_next_buffer
/// @return newly created buffer
rdx_audio_buffer_status rdx_open(rdx_audio_buffer* buf, const char* file, void** out_data, size_t* out_data_size);
/// @brief closes the audio buffer and it's decoding functionality. Does not free resources.
/// @param strm the buffer object to close
/// @return status
rdx_audio_buffer_status rdx_close(rdx_audio_buffer* buf);
/// @brief Writes as much data as possible to the next available buffer
/// @param strm the buffer object
/// @param out_data pointer-to-pointer to write data to. the data is valid for 2 consecutive calls
/// @param out_data_size pointer-to size for the amount of data written
/// @return status
rdx_audio_buffer_status rdx_fill_next_buffer(rdx_audio_buffer* buf, void** out_data, size_t* out_data_size);

/// @brief destroys the buffer object and frees dynamic objects
/// @param buf the buffer object that was previously created with @ref rdx_create_audio_stream
void rdx_destroy_audio_stream(rdx_audio_buffer* buf);

int rdx_get_number_of_channels(rdx_audio_buffer* buf);
int rdx_get_read_interleaved(rdx_audio_buffer* buf);
int rdx_get_sample_rate(rdx_audio_buffer* buf);
rdx_sample_format rdx_get_sample_format(rdx_audio_buffer* buf);
int rdx_get_bits_per_sample(rdx_audio_buffer* buf);
#ifdef __cplusplus
}
#endif
#endif
