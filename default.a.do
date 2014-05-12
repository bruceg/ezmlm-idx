if exists(base+'=l'):
   dependon('makelib')
   objs = [ line.strip() for line in open(base+'=l') ]
   dependon(*objs)
   formake('./makelib {target} {objs}', objs=' '.join(objs))
