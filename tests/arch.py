#!/usr/bin/env python

import random, os, re

test_arch_name = 'arch.tar.bz2'

class ArchTest:
	def createFile(self, name, size):
		data = []
		for i in xrange(0, size):
			data.append(chr(random.randint(0, 255)))
		fp = file(name, 'wb')
		fp.write(''.join(data));
		fp.close()
		
	def createFiles(self):
		names = []
		for i in xrange(10):
			name = 'file%d' % (i + 1)
			self.createFile(name, random.randint(1, 1000000))
			names.append(name)
		return names
		
	def deleteFiles(self, files):
		for file in files:
			try: os.unlink(file)
			except IOError: pass
		
	def calcMD5(self, files):
		md5s = []
		for file in files:
			output = os.popen('md5sum %s' % file).read().strip()
			sr = re.search('(\w+)\s.*', output)
			md5s.append(sr and sr.group(1) or 'invalid file')
		return md5s
	
	def packFiles(self, names, arch_name):
		os.system('tar -cvvpjf %s %s >/dev/null' % (arch_name, ' '.join(names)))
	
	def unpackArch(self, arch_name):
		os.system('tar -xpvvjf %s >/dev/null' % arch_name)
	
	def checkFiles(self, files, md5s):
		new_md5s = self.calcMD5(files)
		for file, md5, new_md5 in zip(files, md5s, new_md5s):
			if md5 != new_md5:
				print '%s md5 should be %s, but it is %s' % (file, md5, new_md5)
				return False
		return True
		
if __name__ == '__main__':
	at = ArchTest()
	print 'creating test files..'
	files = at.createFiles()
	md5s = at.calcMD5(files)
	print 'creating archive..'
	at.packFiles(files, test_arch_name)
	print 'deleting files..'
	at.deleteFiles(files)
	print 'unpacking archive..'
	at.unpackArch(test_arch_name)
	print 'checking if everything is alright..'
	if at.checkFiles(files, md5s): print 'it is'
	print 'cleaning up'
	at.deleteFiles(files)
	at.deleteFiles([test_arch_name])
	
