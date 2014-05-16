def dodir(path, ext):
    dependon(*[ '{}/{}.0'.format(path, filename[:-len(ext)])
                for filename in	listdir(path)
                if filename.endswith(ext) ])

dodir('bin', '.1')
dodir('lib', '.3')

dependon('ezmlm.0', 'ezmlm.0', 'ezmlmglrc.0', 'ezmlmsubrc.0', 'ezmlmrc.0')
