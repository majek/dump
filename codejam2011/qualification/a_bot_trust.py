for case in xrange(input()):
    moves = raw_input().split()[1:]
    assert len(moves) % 2 == 0
    moves.reverse()

    o = b= 1
    ot = bt = 0

    while moves:
        rbt =  moves.pop()
        dst = int(moves.pop())
        if rbt == 'O':
            ot += abs(o - dst) + 1
            o = dst
            if bt >= ot: ot = bt + 1
        else: # 'B'
            bt += abs(b - dst) + 1
            b = dst
            if ot >= bt: bt = ot + 1

    s = max(ot, bt)
    print "Case #%i: %i" % (case+1, s)
