#ifndef BUFFER_H
#define BUFFER_H

/** memory allocation routines */

#include <stdint.h>
#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** allocate buffer of specified size */
void* get_buffer(const size_t size);

/** free buffer */
void free_buffer(void *buffer);

/** append (copy) data to buffer */
off_t pack(const void *data, const size_t size, char *buffer, const off_t offset);

/** append (copy) uint16_t to buffer 
@return offset after appended data
*/
off_t pack_16(const uint16_t *data, char *buffer, const off_t offset);

/** append (copy) uint32_t to buffer 
@return offset after appended data
*/
off_t pack_32(const uint32_t *data, char *buffer, const off_t offset);

/** append (copy) uint64_t to buffer 
@return offset after appended data
*/
off_t pack_64(const uint64_t *data, char *buffer, const off_t offset);

/** get (copy) data from buffer 
@return offset after copied data
*/
off_t unpack(void *data, const size_t size, const char *buffer, const off_t offset);

/** get (copy) uint16_t from buffer 
@return offset after copied data
*/
off_t unpack_16(uint16_t *data, const char *buffer, const off_t offset);

/** get (copy) uint32_t from buffer 
@return offset after copied data
*/
off_t unpack_32(uint32_t *data, const char *buffer, const off_t offset);

/** get (copy) uint64_t from buffer 
@return offset after copied data
*/
off_t unpack_64(uint64_t *data, const char *buffer, const off_t offset);

/** print buffer to output. debug only */
void dump(const char *data, const size_t data_len);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* BUFFER_H */
