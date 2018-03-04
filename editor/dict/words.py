#!/usr/bin/python
# -*- coding: utf-8 -*-

import sqlite3 as lite
import hashlib

word_file = open("words.txt", "r")

con = lite.connect('words.db')
cur = con.cursor()  

cur.execute("CREATE TABLE IF NOT EXISTS words(word TEXT PRIMARY KEY, definition TEXT);")

for word in word_file.read().split():
    if word.upper() != word:
        word = word.lower()
        
    cur.execute("INSERT OR REPLACE INTO words VALUES(\"%s\", \"%s\")" % (word, hashlib.sha256(word).hexdigest()))

word_file.close()

con.commit()

# cur.execute("SELECT word FROM words WHERE word like 'hel%'")
# print cur.fetchall()

con.close()
