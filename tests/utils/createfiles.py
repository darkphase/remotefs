def create_files(number):
    for i in xrange(number):
	open('__%d'% i, 'wt')

if __name__ == '__main__':
    create_files(8000)