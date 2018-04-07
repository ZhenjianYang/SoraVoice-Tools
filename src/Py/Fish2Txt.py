import os, sys
import struct

CODE_PAGE = 'gbk'
CODE_PAGE_OUT = 'utf-8'

def set_code_page(code_page):
    global CODE_PAGE
    CODE_PAGE = code_page

def decode_str(bytes):
    str = bytes.decode(CODE_PAGE)
    ret = ''
    for c in str:
        if ord(c) < 0x20:
            ret += '[x%02X]' % ord(c)
        else:
            ret += c
    return ret

def main():
    istart = 1
    if len(sys.argv) > 1 and sys.argv[1].startswith('--cp='):
        set_code_page(sys.argv[1][5:])
        istart += 1

    if len(sys.argv) < istart + 1:
        print('%s [--cp=<codepage>] T_FISH._DT' % sys.argv[0])
        print('  default codepage = gbk')
        return

    file_in = sys.argv[istart]
    file_name = os.path.split(file_in)[-1]
    name, ext = os.path.splitext(file_name)
    txt_name = name.rstrip(' ') + '.txt'

    file_out = os.path.dirname(file_in) + txt_name

    fs = open(file_in, 'rb')
    fs.seek(8)

    offoff, = struct.unpack('H', fs.read(2))
    fs.seek(offoff)

    offs = []
    while len(offs) == 0 or len(offs) < (offs[0] - offoff) / 2:
        off, = struct.unpack('H', fs.read(2))
        offs.append(off)

    fs_out = open(file_out, mode='w', encoding=CODE_PAGE_OUT)
    fs_out.write('%s\n\n' % file_name)
    for off in offs:
        fs.seek(off)
        bs = b''
        while True:
            b = fs.read(1)
            if b == b'\x00':
                break
            bs += b
        fs_out.write(decode_str(bs))
        fs_out.write('\n\n')
    fs_out.close()

if __name__ == '__main__':
    main()
