#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "nbt.h"
#include "swap.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

#define MAX_CHUNK_SIZE 1024 * 1024

u_int8_t in[MAX_CHUNK_SIZE];
u_int8_t out[MAX_CHUNK_SIZE];

int decompress(u_int8_t* in, u_int8_t* out, size_t length) {
    int ret;
    unsigned have;
    z_stream strm;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = length;
    strm.next_in = in;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */

    /* run inflate() on input until output buffer not full */
    strm.avail_out = MAX_CHUNK_SIZE;
    strm.next_out = out;
    ret = inflate(&strm, Z_FINISH);

    if (ret < Z_OK) {
        printf("Zlib Error %i: %s\n", ret, strm.msg);
    }

    /* clean up and return */
    (void)inflateEnd(&strm);
    return strm.total_out;
}

int parse_type(nbt_state* nbt, int8_t type, bool readStr) {
    u_int8_t str[50];

    switch (type) {
        case NBT_TAG_Byte:
            if (readStr) nbt_skip_string(nbt);
            nbt_read_byte(nbt);
            break;
        case NBT_TAG_Compound:
            if (readStr) nbt_read_string(nbt, str, sizeof str);
            if (readStr) printf("Found compound %s\n", str);
            int8_t tag = nbt_read_byte(nbt);
            while (tag != NBT_TAG_End) {
                parse_type(nbt, tag, true);
                tag = nbt_read_byte(nbt);
            }
            break;
        case NBT_TAG_Short:
            if (readStr) nbt_skip_string(nbt);
            nbt_read_short(nbt);
            break;
        case NBT_TAG_Int:
            if (readStr) nbt_skip_string(nbt);
            nbt_read_int(nbt);
            break;
        case NBT_TAG_Long:
            if (readStr) nbt_skip_string(nbt);
            nbt_read_long(nbt);
            break;
        case NBT_TAG_Float:
            if (readStr) nbt_skip_string(nbt);
            nbt_read_float(nbt);
            break;
        case NBT_TAG_Double:
            if (readStr) nbt_skip_string(nbt);
            nbt_read_float(nbt);
            break;
        case NBT_TAG_Byte_Array:
            if (readStr) nbt_skip_string(nbt);
            nbt_skip_byte_array(nbt);
            break;
        case NBT_TAG_Int_Array:
            if (readStr) nbt_skip_string(nbt);
            nbt_skip_int_array(nbt);
            break;
        case NBT_TAG_Long_Array:
            if (readStr) nbt_skip_string(nbt);
            nbt_skip_long_array(nbt);
            break;
        case NBT_TAG_List:
            if (readStr) nbt_read_string(nbt, str, sizeof str);
            nbt_list list;
            nbt_read_list(nbt, &list);
            printf("Found list %s\n", str);
            if (!strcmp(str, "block_entities")) {
                printf("Found the one! %s\n", str);
                return -1;
            } else {
                for (int i = 0; i < list.len; i++) {
                    parse_type(nbt, list.type, false);
                }
            }
            break;
        default:
            printf("Error unknown tag %i\n", type);
            break;
    }
    return 0;
}

void walk_nbt(u_int8_t* data, int size) {
    nbt_state nbt;
    nbt.buf = data;
    nbt.ptr = 0;

    nbt.ptr += 1;

    nbt_skip_string(&nbt);
    while (nbt.ptr < size) {
        int8_t type = nbt_read_byte(&nbt);
        if (parse_type(&nbt, type, true) < 0) {
            return;
        }
        printf("%i\n", nbt.ptr);
    }
}

int main() {
    FILE* fp;

    int last_timestamp = 0;

    fp = fopen("r.0.0.mca", "r");  // read mode

    if (fp == NULL) {
        perror("Error while opening the file.\n");
        exit(EXIT_FAILURE);
    }

    u_int8_t header[1024 * 8];

    int r = fread(header, sizeof header[0], sizeof header, fp);

    int count;

    for (int x = 0; x < 1; x++) {
        for (int z = 1; z < 2; z++) {
            int h_off = 4 * ((x & 31) + (z & 31) * 32);
            int timestamp = swap_int32(*(int*)(header + h_off + 4096));
            if (timestamp > last_timestamp) {
                u_int8_t* header_at_off = header + h_off;
                int off = 4096 * ((((header_at_off[0] << 16) + header_at_off[1]) << 8) + header_at_off[2]);
                fseek(fp, off, SEEK_SET);

                uint32_t size_buf = 0;
                fread(&size_buf, sizeof size_buf, 1, fp);
                int size = swap_uint32(size_buf);

                fseek(fp, 1, SEEK_CUR);

                if (size - 1 > MAX_CHUNK_SIZE) {
                    printf("Error chunk size bigger then 1 MiB\n");
                    return 1;
                }

                fread(in, sizeof in[0], size - 1, fp);

                decompress(in, out, size);

                walk_nbt(out, size);
            }
        }
    }

    fclose(fp);
    return 0;
}