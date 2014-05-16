dependon(*[ 'bin/' + filename[:-2]
            for filename in listdir('bin')
            if filename.endswith('=x') ])
dependon(
'bin/ezmlm-test',
'db/sub-std.so',
'ezmlmrc.all',
'messages.all',
'ezmlm-idx.spec',
'test-getconfopt')
