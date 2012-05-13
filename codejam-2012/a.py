
a = "ejp mysljylc kd kxveddknmc re jsicpdrysirbcpc ypc rtcsra dkh wyfrepkym veddknkmkrkcdde kr kd eoya kw aej tysr re ujdr lkgc jv"
b = "our language is impossible to understandthere are twenty six factorial possibilitiesso it is okay if you want to just give up"

chars = {' ':' ', 'z':'q', 'a':'y', 'o':'e', 'q':'z'}
for i in range(len(a)):
    if a[i] == ' ' and b[i] == ' ': continue
    chars[a[i]] = b[i]

#print sorted(chars.values())

exit
for test_no in range(1, input()+1):
    line = raw_input()
    trans = [chars[c] for c in line]
    print "Case #%s: %s" % (test_no, ''.join(trans))
