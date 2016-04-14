for lang in listdir('lang'):
    if exists('lang/{}/messages'.format(lang)):
	    dependon('lang/{}/text/messages'.format(lang))
