import sys, os
import codecs
from collections import defaultdict

DICT_FILE_FORMAT = 'LGCStringDict_%02d.txt'
DICT_MAX_NUM = 4
CODEPAGE_PY = 'utf8'
CODEPAGE_TXT = 'gbk'
CODEPAGE_TXT_EN = 'ms932'

DICT_ITEM_NO = b'xfhsm_res_No.'
DICT_ITEM_ENG_START = b'xfhsm_res_ENG_Start'
DICT_ITEM_ENG_END = b'xfhsm_res_ENG_End'
DICT_ITEM_CHI_START = b'xfhsm_res_CHI_Start'
DICT_ITEM_CHI_END = b'xfhsm_res_CHI_End'
DICT_ENG = b'ENG'
DICT_CHI = b'CHI'
DICT_MARK = 'xf'

log = print

RAW_STRING_MODE=False

def getSubFiles(dir, ext = '.py'):
    ret = []
    dir_list =  os.listdir(dir)
    for file in dir_list:
        path = os.path.join(dir, file)
        if os.path.splitext(file)[1] == ext and os.path.isfile(path):
            ret.append(file)
    return ret

def getDict(path):
    file = open(path, 'rb')
    data = file.read()
    file.close()

    lines = data.split(b'\x0D\x0A')
    lines.append(DICT_ITEM_NO + b'-1')
    ret = {}
    xfNo = -1
    xfDict = defaultdict(str)
    xfLang = ''
    cp = ''
    for line in lines:
        if line.startswith(DICT_ITEM_NO):
            if xfNo >= 0:
                ret[xfNo] = xfDict
            xfNo = int(line[len(DICT_ITEM_NO):])
            xfDict = defaultdict(str)
        else:
            if line.startswith(DICT_ITEM_ENG_START):
                xfLang = DICT_ENG
                cp = CODEPAGE_TXT_EN
            elif line.startswith(DICT_ITEM_CHI_START):
                xfLang = DICT_CHI
                cp = CODEPAGE_TXT
            elif line.startswith(DICT_ITEM_ENG_END) or line.startswith(DICT_ITEM_CHI_END):
                xfLang = ''
            elif xfLang:
                xfDict[xfLang] += line.decode(cp, errors='replace')

    return ret

def getDicts(dir):
    ret = [{} for _ in range(DICT_MAX_NUM)]
    for iDict in range(DICT_MAX_NUM):
        dictFile = DICT_FILE_FORMAT % iDict
        path = os.path.join(dir, dictFile)
        if os.path.isfile(path):
            log('Reading Dictfile %s ... ' % dictFile, end='', flush=True)
            ret[iDict] = getDict(path)
            log('Done. %d strings.' % len(ret[iDict]))
    return ret

def formatWithDict(text, dicts):
    text_new = ''
    i = 0
    while i < len(text):
        if text[i] == '|':
            text_new += bytes.fromhex(text[i+1:i+5]).decode(CODEPAGE_TXT)
            i += 5
        elif text[i:i+2] == DICT_MARK:
            iDict = int(text[i+2])
            sNo = ''
            j = i + 3
            while j < len(text) and '0' <= text[j] <= '9':
                sNo += text[j]
                j += 1
            while j < len(text) and text[j] == 'W':
                j += 1
            No = int(sNo)
            if No in dicts[iDict]:
                text_new += dicts[iDict][No][DICT_CHI]
            else:
                log('Wanning: not found: ' + text[i:j])
                text_new += text[i:j]
            i = j
        else:
            text_new += text[i]
            i += 1
    return text_new

def restorteDict(path_in, path_out, dicts):
    file_in = codecs.open(path_in, 'r', CODEPAGE_PY)
    lines = file_in.readlines()
    file_in.close()

    with codecs.open(path_out, 'w', CODEPAGE_PY) as file_out:
        for line in lines:
            newLine = ''
            
            if RAW_STRING_MODE:
                newLine = formatWithDict(line, dicts)
            else:
                i, ip = 0, 0
                while i < len(line):
                    if line[i] in '\'"':
                        j = line.find(line[i], i + 1)
                        newLine += line[ip:i+1]
                        newLine += formatWithDict(line[i+1:j], dicts)
                        ip = j
                        i = j + 1
                    else:
                        i += 1
                newLine += line[ip:]
            file_out.write(newLine)

def main():
    params = []
    switches = {}
    for i in range(1, len(sys.argv)):
        if sys.argv[i] and sys.argv[i][0:2] == '--':
            je = sys.argv[i].find('=')
            if je < 0: je = len(sys.argv[i])
            switches[sys.argv[i][2:je]] = sys.argv[i][je+1:]
        else:
            params.append(sys.argv[i])
    if len(params) < 2:
        print('Usage:')
        print('    %s [--cp=<codepage>] [--cpen=<codepage>] [--rs] dir_in dir_dict [dir_out]' % sys.argv[0])
        return
    
    dir_in = params[0]
    dir_dict = params[1]
    dir_out = params[2] if len(params) > 2 else dir_in + '.out'

    global CODEPAGE_TXT, CODEPAGE_TXT_EN, RAW_STRING_MODE
    if 'cp' in switches:
        cp = switches['cp']
        CODEPAGE_TXT = cp
    if 'cpen' in switches:
        cp = switches['cpen']
        CODEPAGE_TXT_EN = cp
    if 'rs' in switches:
        RAW_STRING_MODE = True

    log('Reading Dict files...')
    dicts = getDicts(dir_dict)

    log('Now going to deal with files...')
    if not os.path.exists(dir_out):
        os.makedirs(dir_out)
    pys = getSubFiles(dir_in, '.py') + getSubFiles(dir_in, '.txt')
    log('%d files.' % len(pys))
    for py in pys:
        log('Working with %s...' % py, end='', flush=True)
        path_in = os.path.join(dir_in, py)
        path_out = os.path.join(dir_out, py)
        restorteDict(path_in, path_out, dicts)
        log('Done.')

    log('All Done.')

if __name__ == '__main__':
    main()