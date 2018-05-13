import os, sys
import struct

CODE_PAGE = 'gbk'
CODE_PAGE_OUT = 'utf8'

def set_code_page(code_page):
    global CODE_PAGE
    CODE_PAGE = code_page

def main():
    reverse = False
    istart = 1
    if len(sys.argv) > istart and sys.argv[istart].startswith('--cp='):
        set_code_page(sys.argv[istart][5:])
        istart += 1
    if len(sys.argv) > istart and sys.argv[istart] == '-r':
        reverse = True
        istart += 1

    if len(sys.argv) < istart + 3:
        print('%s [--cp=<codepage>] [-r] binfile start end rep_time [base] [output file]' % sys.argv[0])
        print('  default base = 0')
        print('  default output file = binfile.txt')
        return

    filein = sys.argv[istart + 0];
    fileout = filein + '.txt' if len(sys.argv) <= istart + 5 else sys.argv[istart + 5]
    start = int(sys.argv[istart + 1], 16) if sys.argv[istart + 1].lower().startswith('0x') else int(sys.argv[istart + 1])
    end = int(sys.argv[istart + 2], 16) if sys.argv[istart + 2].lower().startswith('0x') else int(sys.argv[istart + 2])
    rep_time = int(sys.argv[istart + 3])
    base = '0' if len(sys.argv) <= istart + 4 else sys.argv[istart + 4]
    base = int(base, 16) if base.lower().startswith('0x') else int(base)

    fs = open(filein, 'rb')
    buff = fs.read()
    fs.close()
    buff = buff[start:end]

    strs = buff.split(b'\0')
    i = start

    outs = []

    for s in strs:
        if i % 4 == 0 and s:
            outs.append((i + base, str(s, encoding=CODE_PAGE).replace('\n','\\n').replace('%c%c','%c%c\n')))
        i += len(s) + 1
    
    if reverse:
        outs.reverse()

    fs_out = open(fileout, mode='w', encoding=CODE_PAGE_OUT)
    for s in outs:
        for j in range(rep_time):
            fs_out.write('%08X\n' % s[0])
            fs_out.write('.%s\n' % CODE_PAGE)
            fs_out.write(s[1])
            fs_out.write('\n\n')
    fs_out.close()

if __name__ == '__main__':
    main()

