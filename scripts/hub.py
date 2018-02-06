import csv
with open('../peer.csv') as csvfile:
     reader = csv.DictReader(csvfile)
     for row in reader:
         if row['ID'] == 15:
             row['ID'] = 77

#print "hello world!"
