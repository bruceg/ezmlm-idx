for section in (1,2,3,4,5,6,7,8):
    fn = '{}.{}'.format(base, section)
    if exists(fn):
        dependon(fn)
        formake('nroff -man {fn} > {target}', fn=fn)
        break
