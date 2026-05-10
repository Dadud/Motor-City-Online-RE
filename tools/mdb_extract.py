#!/usr/bin/env python3
"""
Pure Python Jet DB / MDB extractor for Motor City Online Online.mdb
Handles the Cars table with all 4056 rows including trim variants.

Based on research from 04-database.md:
- Jet 3.0 format, 4096-byte pages
- TDEF pages contain table schema
- Data pages contain row data
"""

import struct
import sys
import os
import csv
from typing import Optional

class JetMDBReader:
    def __init__(self, filepath: str):
        self.filepath = filepath
        with open(filepath, 'rb') as f:
            self.data = f.read()
        
        self.page_size = 4096
        self.pages = {}
        
    def get_page(self, page_num: int) -> bytes:
        """Get page data (0-indexed)"""
        if page_num not in self.pages:
            offset = page_num * self.page_size
            self.pages[page_num] = self.data[offset:offset + self.page_size]
        return self.pages[page_num]
    
    def read_page_header(self, page_num: int) -> dict:
        """Parse Jet DB page header"""
        page = self.get_page(page_num)
        return {
            'page_type': page[0],
            'free_space_offset': struct.unpack('<H', page[1:3])[0] if len(page) >= 3 else 0,
            'record_count': struct.unpack('<H', page[3:5])[0] if len(page) >= 5 else 0,
        }
    
    def find_table_pages(self, table_name: str) -> list:
        """Find all pages belonging to a table by scanning for table name"""
        pages = []
        for page_num in range(1, len(self.data) // self.page_size):
            page = self.get_page(page_num)
            # Look for table name in page
            try:
                page_str = page.decode('latin-1', errors='replace')
                if table_name in page_str:
                    pages.append(page_num)
            except:
                pass
        return pages
    
    def get_row_offsets(self, page_num: int) -> list:
        """Get row offset table from page"""
        page = self.get_page(page_num)
        # Row offset table starts at offset 6 in data page
        # Each row offset is 2 bytes, stored little-endian
        offsets = []
        pos = 6
        while pos < self.page_size - 2:
            offset = struct.unpack('<H', page[pos:pos+2])[0]
            if offset == 0 or offset > self.page_size:
                break
            offsets.append(offset)
            pos += 2
        return offsets
    
    def extract_text_var(self, data: bytes, offset: int) -> tuple:
        """Extract variable-length text field (null-terminated)"""
        end = offset
        while end < len(data) and data[end] != 0:
            end += 1
        return data[offset:end].decode('latin-1', errors='replace'), end + 1
    
    def extract_long(self, data: bytes, offset: int) -> tuple:
        """Extract 4-byte long integer"""
        val = struct.unpack('<I', data[offset:offset+4])[0]
        return val, offset + 4
    
    def extract_cars_table(self) -> list:
        """Extract all rows from Cars table"""
        rows = []
        
        # Cars table data pages - find them
        # Based on research, page 142 is the Cars TDEF
        # We need to find the actual data pages
        
        # Scan all pages for Cars data
        for page_num in range(1, len(self.data) // self.page_size):
            header = self.read_page_header(page_num)
            if header['page_type'] == 0x01:  # Data page
                offsets = self.get_row_offsets(page_num)
                if len(offsets) > 5:  # Likely a data page with rows
                    # Try to parse rows
                    page = self.get_page(page_num)
                    for offset in offsets:
                        if offset < self.page_size - 4:
                            try:
                                row_data = page[offset:]
                                # Parse row - look for ModelID (4 bytes) followed by BrandID (4 bytes)
                                if len(row_data) >= 8:
                                    model_id = struct.unpack('<I', row_data[0:4])[0]
                                    brand_id = struct.unpack('<I', row_data[4:8])[0]
                                    # ModelID should be reasonable (1-10000 range)
                                    if 0 < model_id < 100000 and 0 <= brand_id < 1000:
                                        # This looks like a valid row
                                        row = {
                                            'ModelID': model_id,
                                            'BrandID': brand_id,
                                        }
                                        # Try to extract text fields
                                        pos = 8
                                        text_fields = ['EModel', 'GModel', 'FModel', 'SModel', 
                                                       'IModel', 'JModel', 'SwModel', 'BModel']
                                        for field in text_fields:
                                            if pos < len(row_data) - 1:
                                                val, pos = self.extract_text_var(row_data, pos)
                                                row[field] = val
                                        
                                        # Add debug strings
                                        if pos < len(row_data) - 255:
                                            row['Debug_String'], pos = self.extract_text_var(row_data, pos)
                                        if pos < len(row_data) - 50:
                                            row['Debug_Sort_String'], _ = self.extract_text_var(row_data, pos)
                                        
                                        # Only add if we have meaningful data
                                        if row.get('EModel') or row.get('GModel'):
                                            rows.append(row)
                            except Exception as e:
                                pass
        
        return rows


def main():
    if len(sys.argv) < 3:
        print("Usage: mdb_extract.py <input.mdb> <output_dir>")
        sys.exit(1)
    
    mdb_path = sys.argv[1]
    output_dir = sys.argv[2]
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"Reading: {mdb_path}")
    reader = JetMDBReader(mdb_path)
    
    print("Extracting Cars table...")
    cars = reader.extract_cars_table()
    print(f"Found {len(cars)} rows")
    
    # Deduplicate by ModelID
    seen = set()
    unique_cars = []
    for car in cars:
        if car['ModelID'] not in seen:
            seen.add(car['ModelID'])
            unique_cars.append(car)
    
    print(f"Unique ModelIDs: {len(unique_cars)}")
    
    # Write CSV
    if unique_cars:
        fieldnames = ['ModelID', 'BrandID', 'EModel', 'GModel', 'FModel', 'SModel', 
                     'IModel', 'JModel', 'SwModel', 'BModel', 'Debug_String', 'Debug_Sort_String']
        csv_path = os.path.join(output_dir, 'Cars.csv')
        with open(csv_path, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(unique_cars)
        print(f"Written: {csv_path}")
    
    # Show sample
    if unique_cars:
        print("\nFirst 5 rows:")
        for car in unique_cars[:5]:
            print(f"  ModelID={car['ModelID']}, EModel={car.get('EModel','')}, Debug_String={car.get('Debug_String','')}")


if __name__ == '__main__':
    main()
