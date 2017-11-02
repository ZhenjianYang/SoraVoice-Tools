import os, sys
import codecs
import struct

CODE_PAGE = 'gbk'
def set_code_page(code_page):
    global CODE_PAGE
    CODE_PAGE = code_page

def decode(bytes):
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

    if len(sys.argv) <= istart:
        print('%s [--cp=<codepage>] XXXX._DT [output folder]' % sys.argv[0])
        print('  default codepage = gbk')
        return

    path = sys.argv[istart]
    dir_out = os.path.dirname(path) if len(sys.argv) <= istart + 1 else sys.argv[istart + 1]
    if len(dir_out) == 0:
        dir_out = '.'
    file_name = os.path.split(path)[-1]
    name, ext = os.path.splitext(file_name)
    txt_name = name + '.txt'

    fs = open(path, 'rb')
    buff = fs.read()
    fs.close()

    offs = []
    i = 0
    while len(offs) == 0 or i < offs[0]:
        offs.extend(struct.unpack('H', buff[i:i+2]))
        i += 2

    if not os.path.exists(dir_out):
        os.makedirs(dir_out)
    fs_out = codecs.open(os.path.join(dir_out, txt_name), 'w', CODE_PAGE)
    fs_out.write('%s\n' % file_name)

    for off in offs:
        no, ans = struct.unpack('HH', buff[off:off+4])
        sub_offs = struct.unpack('HHHHHH', buff[off+4:off+16])

        fs_out.write('\n')
        fs_out.write('%d\n' % no)
        fs_out.write('%d\n' % ans)
        for begin in sub_offs:
            end = begin
            while buff[end] != 0:
                end += 1
            fs_out.write('%s\n' % decode(buff[begin:end]))

    fs_out.close()

if __name__ == '__main__':
    main()
