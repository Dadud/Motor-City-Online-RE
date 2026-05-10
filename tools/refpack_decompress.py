#!/usr/bin/env python3
"""
RefPack decompressor for EA games.
Based on the C implementation from Niotso Wiki:
http://wiki.niotso.org/RefPack

RefPack is an LZ77/LZSS compression format written by Frank Barchard of EA Canada.
Used in many EA games including Motor City Online.
"""

import struct
import sys
from typing import Tuple, Optional

class RefPackDecompressor:
    """RefPack decompression implementation."""
    
    def decompress(self, data: bytes, max_out_size: int = 0) -> Tuple[bytes, int]:
        """
        Decompress RefPack data.
        
        Args:
            data: Compressed bytes
            max_out_size: Maximum output buffer size (0 = use decompressed_size from header)
            
        Returns:
            (decompressed_data, bytes_consumed)
        """
        if len(data) < 5:
            raise ValueError(f"Data too short: {len(data)} bytes (minimum 5)")
        
        inp = memoryview(data)
        outp = bytearray()
        
        # Read header
        signature = (inp[0] << 8) | inp[1]
        pos = 2
        
        has_compressed_size = signature & 0x0100
        large_files = signature & 0x8000  # Flag L
        
        if has_compressed_size:
            if large_files:
                compressed_size = struct.unpack('>I', inp[pos:pos+4])[0]
                pos += 4
            else:
                compressed_size = (inp[pos] << 16) | (inp[pos+1] << 8) | inp[pos+2]
                pos += 3
        
        if large_files:
            decompressed_size = struct.unpack('>I', inp[pos:pos+4])[0]
            pos += 4
        else:
            decompressed_size = (inp[pos] << 16) | (inp[pos+1] << 8) | inp[pos+2]
            pos += 3
        
        if max_out_size > 0 and decompressed_size > max_out_size:
            decompressed_size = max_out_size
        
        # Decompress
        out_buf = bytearray(decompressed_size)
        out_pos = 0
        
        while out_pos < decompressed_size:
            if pos >= len(data):
                break
                
            byte0 = inp[pos]
            pos += 1
            
            if not (byte0 & 0x80):
                # 2-byte command: 0DDRRRPP DDDDDDDD
                if pos >= len(data):
                    break
                byte1 = inp[pos]
                pos += 1
                
                proc_len = byte0 & 0x03
                for i in range(proc_len):
                    if out_pos >= decompressed_size:
                        break
                    out_buf[out_pos] = inp[pos]
                    pos += 1
                    out_pos += 1
                
                ref_dis = ((byte0 & 0x60) << 3) + byte1 + 1
                ref_len = ((byte0 >> 2) & 0x07) + 3
                
                for i in range(ref_len):
                    if out_pos >= decompressed_size:
                        break
                    src_pos = out_pos - ref_dis
                    if src_pos < 0:
                        src_pos = 0
                    out_buf[out_pos] = out_buf[src_pos]
                    out_pos += 1
                    
            elif not (byte0 & 0x40):
                # 3-byte command: 10RRRRRR PPDDDDDD DDDDDDDD
                if pos + 1 >= len(data):
                    break
                byte1 = inp[pos]
                byte2 = inp[pos + 1]
                pos += 2
                
                proc_len = byte1 >> 6
                for i in range(proc_len):
                    if out_pos >= decompressed_size:
                        break
                    out_buf[out_pos] = inp[pos]
                    pos += 1
                    out_pos += 1
                
                ref_dis = ((byte1 & 0x3f) << 8) + byte2 + 1
                ref_len = (byte0 & 0x3f) + 4
                
                for i in range(ref_len):
                    if out_pos >= decompressed_size:
                        break
                    src_pos = out_pos - ref_dis
                    if src_pos < 0:
                        src_pos = 0
                    out_buf[out_pos] = out_buf[src_pos]
                    out_pos += 1
                    
            elif not (byte0 & 0x20):
                # 4-byte command: 110DRRPP DDDDDDDD DDDDDDDD RRRRRRRR
                if pos + 2 >= len(data):
                    break
                byte1 = inp[pos]
                byte2 = inp[pos + 1]
                byte3 = inp[pos + 2]
                pos += 3
                
                proc_len = byte0 & 0x03
                for i in range(proc_len):
                    if out_pos >= decompressed_size:
                        break
                    out_buf[out_pos] = inp[pos]
                    pos += 1
                    out_pos += 1
                
                ref_dis = ((byte0 & 0x10) << 12) + (byte1 << 8) + byte2 + 1
                ref_len = ((byte0 & 0x0c) << 6) + byte3 + 5
                
                for i in range(ref_len):
                    if out_pos >= decompressed_size:
                        break
                    src_pos = out_pos - ref_dis
                    if src_pos < 0:
                        src_pos = 0
                    out_buf[out_pos] = out_buf[src_pos]
                    out_pos += 1
                    
            else:
                # 1-byte command: 111PPPPP
                proc_len = (byte0 & 0x1f) * 4 + 4
                
                if proc_len <= 0x70:
                    # No stop flag
                    for i in range(proc_len):
                        if out_pos >= decompressed_size:
                            break
                        out_buf[out_pos] = inp[pos]
                        pos += 1
                        out_pos += 1
                else:
                    # Stop flag
                    proc_len = byte0 & 0x03
                    for i in range(proc_len):
                        if out_pos >= decompressed_size:
                            break
                        out_buf[out_pos] = inp[pos]
                        pos += 1
                        out_pos += 1
                    break  # End of stream
        
        return bytes(out_buf[:out_pos]), pos
    
    def decompress_simple(self, data: bytes) -> bytes:
        """Simple decompress that returns just the decompressed data."""
        result, _ = self.decompress(data)
        return result


def test_refpack():
    """Test with known RefPack data."""
    # Test with a simple pattern
    pass


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: refpack_decompress.py <input_file> [output_file]")
        sys.exit(1)
    
    with open(sys.argv[1], 'rb') as f:
        compressed = f.read()
    
    dec = RefPackDecompressor()
    try:
        decompressed, consumed = dec.decompress(compressed)
        print(f"Compressed size: {len(compressed)}")
        print(f"Decompressed size: {len(decompressed)}")
        print(f"Bytes consumed: {consumed}")
        
        if len(sys.argv) >= 3:
            with open(sys.argv[2], 'wb') as f:
                f.write(decompressed)
            print(f"Wrote {len(decompressed)} bytes to {sys.argv[2]}")
        else:
            # Dump first 256 bytes as hex
            print("\nFirst 256 bytes (hex):")
            for i in range(0, min(256, len(decompressed)), 16):
                hex_str = ' '.join(f'{b:02x}' for b in decompressed[i:i+16])
                print(f'{i:04x}: {hex_str}')
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
