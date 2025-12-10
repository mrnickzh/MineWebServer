#pragma once

#include <vector>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <zlib.h>

class ZLibUtils {
public:
    static std::vector<uint8_t> compress_data(const std::vector<uint8_t>& inbuf,
                                int compressionlevel = Z_BEST_COMPRESSION)
    {
        z_stream zs;                        // z_stream is zlib's control structure
        memset(&zs, 0, sizeof(zs));

        int err = deflateInit(&zs, compressionlevel);
        if (err != Z_OK)
            throw(std::runtime_error("deflateInit error " + std::to_string(err)));

        zs.next_in = (Bytef*)inbuf.data();
        zs.avail_in = inbuf.size();           // set the z_stream's input

        int ret;
        char outbuffer[32768];
        std::vector<uint8_t> outbuf;

        // retrieve the compressed bytes blockwise
        do {
            zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
            zs.avail_out = sizeof(outbuffer);

            ret = deflate(&zs, Z_FINISH);

            if (outbuf.size() < zs.total_out) {
                outbuf.insert(outbuf.end(), outbuffer, outbuffer + (zs.total_out - outbuf.size()));
            }
        } while (ret == Z_OK);

        deflateEnd(&zs);

        if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
            std::ostringstream oss;
            oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
            throw std::runtime_error(oss.str());
        }

        return outbuf;
    }

    static std::vector<uint8_t> decompress_data(const std::vector<uint8_t>& inbuf)
    {
        z_stream zs;                        // z_stream is zlib's control structure
        memset(&zs, 0, sizeof(zs));

        int err = inflateInit(&zs);
        if (err != Z_OK)
            throw(std::runtime_error("inflateInit error " + std::to_string(err)));

        std::cout << inbuf.size() << " size" << std::endl;

        zs.next_in = (Bytef*)inbuf.data();
        zs.avail_in = inbuf.size();

        int ret;
        char outbuffer[32768];
        std::vector<uint8_t> outbuf;

        // get the decompressed bytes blockwise using repeated calls to inflate
        do {
            zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
            zs.avail_out = sizeof(outbuffer);

            ret = inflate(&zs, 0);

            if (outbuf.size() < zs.total_out) {
                outbuf.insert(outbuf.end(), outbuffer, outbuffer + (zs.total_out - outbuf.size()));
            }

        } while (ret == Z_OK);

        inflateEnd(&zs);

        if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
            std::ostringstream oss;
            oss << "Exception during zlib decompression: (" << ret << ") " << zs.msg;
            throw std::runtime_error(oss.str());
        }

        return outbuf;
    }
};