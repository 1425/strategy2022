#!/usr/bin/env python3

from bs4 import BeautifulSoup
from urllib.request import urlopen
from argparse import ArgumentParser

def firsts(a):
	return map(lambda x: x[0],a)

def seconds(a):
	return map(lambda x: x[1],a)

def show(x):
	if type(x) is list:
		for i,x in enumerate(x):
			print (i,str(x)[:100])
	else:
		print(str(x)[:500])

def print_lines(x):
	for a in x:
		print(a)

def parse(path):
	#Returns maybe [[str]]

	#From https://scout.valor6800.com/all.html
	#This is not automatically downloaded since it is populated by javascript
	#And doesn't have any data in it if just downloaded directly

	soup=BeautifulSoup(open(path),'html.parser')
	x=soup.html
	if x is None:
		return x

	table=x.find_all('table')[0]
	x=table.thead.tr

	def parse_heading(y):
		inner=y.string
		if inner is None:
			return 'OPR'
		return inner

	headings=list(map(parse_heading,x.find_all('td')))

	def parse_row(row):
		return list(map(
			lambda y: (y[0],y[1].string),
			zip(headings,row.find_all('td'))
		))

	return list(map(parse_row,table.tbody.find_all('tr')))

def output(p):
	#going to output csv.

	if p is None:
		return

	print(','.join(firsts(p[0])))
	for row in p:
		print(','.join(seconds(row)))

def main():
	a=ArgumentParser()
	a.add_argument('--path',default='data/Valor Scout.html')
	args=a.parse_args()
	p=parse(args.path)
	output(p)

if __name__=='__main__':
	main()
