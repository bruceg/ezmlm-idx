if exists(target+'=x'):
    dependx(target+'=x','load',target+'.o')

elif exists(target+'=s'):
    dependon('warn-auto.sh',target+'.sh')
    formake('cat warn-auto.sh {target}.sh > {target}')
    formake('chmod 755 {target}')

elif exists(target+'.template'):
    dependon(target+'.template', 'VERSION', 'fill-template.sh')
    formake('sh fill-template.sh {target}')

elif target in ['compile', 'libcompile', 'load', 'makelib', 'makeso']:
    dependon('build/make-{}.sh'.format(target), 'warn-auto.sh', 'systype', 'conf-cc', 'conf-cclo', 'conf-ld', 'conf-ldso')
    formake('( cat warn-auto.sh; sh build/make-{target}.sh "`cat systype`" ) > {target}')
    formake('chmod 755 {target}')

elif target.startswith('lang/') and target.endswith('/ezmlmrc'):
    lang = target[5:-8]
    dependon('makelang.sh','ezmlmrc')
    formake('sh makelang.sh '+lang)

elif target.startswith('lang/') and target.endswith('/text/messages'):
    lang = target[5:-14]
    dependon('build/make-messages.sh','lang/{}/messages'.format(lang))
    formake('sh build/make-messages.sh < lang/{lang}/messages > lang/{lang}/text/messages', lang=lang)

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
