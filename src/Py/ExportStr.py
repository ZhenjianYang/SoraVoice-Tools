import os, sys
import struct

CODE_PAGE = 'gbk'

def set_code_page(code_page):
    global CODE_PAGE
    CODE_PAGE = code_page

def main():
    istart = 1
    if len(sys.argv) > 1 and sys.argv[1].startswith('--cp='):
        set_code_page(sys.argv[1][5:])
        istart += 1

    if len(sys.argv) < istart + 3:
        print('%s [--cp=<codepage>] binfile start end [base] [output file]' % sys.argv[0])
        print('  default base = 0')
        print('  default output file = binfile.txt')
        return

    filein = sys.argv[istart + 0];
    fileout = filein + '.txt' if len(sys.argv) <= istart + 4 else sys.argv[istart + 4]
    start = int(sys.argv[istart + 1], 16) if sys.argv[istart + 1].lower().startswith('0x') else int(sys.argv[istart + 1])
    end = int(sys.argv[istart + 2], 16) if sys.argv[istart + 2].lower().startswith('0x') else int(sys.argv[istart + 2])
    base = '0' if len(sys.argv) <= istart + 3 else sys.argv[istart + 3]
    base = int(base, 16) if base.lower().startswith('0x') else int(base)

    fs = open(filein, 'rb')
    buff = fs.read()
    fs.close()
    buff = buff[start:end]

    strs = buff.split(b'\0')
    i = start

    fs_out = open(fileout, mode='w', encoding=CODE_PAGE)
    for s in strs:
        if i % 4 == 0:
            fs_out.write('%08X\n' % (i + base))
            fs_out.write('.%s\n' % CODE_PAGE)
            fs_out.write(str(s, encoding=CODE_PAGE))
            fs_out.write('\n')
        i += len(s) + 1
    fs_out.close()

if __name__ == '__main__':
    main()

