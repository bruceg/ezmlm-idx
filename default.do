if exists(target+'=x'):
    dependx(target+'=x','load',target+'.o')

elif exists(target+'=s'):
    dependon('warn-auto.sh',target+'.sh')
    formake('cat warn-auto.sh {target}.sh > {target}')
    formake('chmod 755 {target}')

elif exists(target+'.template'):
    dependon(target+'.template', 'VERSION', 'fill-template')
    formake('./fill-template {target}')

elif target in ['compile', 'libcompile', 'load', 'makelib', 'makeso']:
    dependon('make-'+target, 'warn-auto.sh', 'systype')
    formake('( cat warn-auto.sh; ./make-{target} "`cat systype`" ) > {target}')
    formake('chmod 755 {target}')

elif target.startswith('make-'):
    dependon(target+'.sh', 'auto-ccld.sh')
    formake('cat auto-ccld.sh {target}.sh > {target}')
    formake('chmod 755 {target}')

elif target.startswith('lang/') and target.endswith('/ezmlmrc'):
    lang = target[5:-8]
    dependon('makelang','ezmlmrc')
    formake('./makelang '+lang)

elif target.startswith('lang/') and target.endswith('/text/messages'):
    lang = target[5:-14]
    dependon('make-messages','lang/{}/messages'.format(lang))
    formake('./make-messages < lang/{lang}/messages > lang/{lang}/text/messages', lang=lang)

elif target == 'lib/auto_version.c':
    dependon('auto-str','VERSION')
    formake('./auto-str auto_version < VERSION > {target}')

elif target in ['lib/auto_bin.c', 'lib/auto_lib.c', 'lib/auto_etc.c']:
    base = target[9:-2]
    dependon('auto-str','conf-'+base)
    formake('./auto-str auto_{base} EZMLM_{ubase} < conf-{base} > {target}', base=base, ubase=base.upper())

elif target.startswith('lib/auto_') and target.endswith('.c'):
    base = target[9:-2]
    dependon('auto-str','conf-'+base)
    formake('./auto-str auto_{base} < conf-{base} > {target}', base=base)
