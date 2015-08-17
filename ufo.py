import collections

hours = 60 * 60
days = 24 * hours

Item = collections.namedtuple(
    'Item', ['after', 'name', 'descr', 'dur'])

raw_items = [
    ([], 'start', "1st Jan 1999", 0),

    ('start', 'r_ms', "LAB Motion Scanner", 7 * days),
    ('r_ms', 'r_lw', "LAB Laser Weapons", 0 * days),
    ('r_lw', 'r_lp', "LAB Laser Pistol", 0 * days),
    ('r_lp', 'r_lr', "LAB Laser Rifle", 0 * hours),
    ('r_lr', 'r_lh', "LAB Heavy Laser", 0 * hours),
    ('r_lh', 'r_lc', "LAB Laser Cannon", 0 * hours),
    ('start', 'ec_lrs', "BUILD Large Radar System", 25 * days),
    ('start', 'ec_lq1', "BUILD Living Quarters #2", 16 * days),
    ('start', 'ec_ac', "BUILD Alien Containment", 18 * days),
    ('ec_ac', 'ec_gs2', "BUILD General Stores #2", 10 * days),
    ('start', 'b_scie1', "BUY 20 scientists", 72 * hours),

    ('ec_lq1', 'b_en1', "BUY 36 engineers", 4 * days),
    ('ec_lq1', 'b_so1', "BUY 4 soldiers", 4 * days),
    ('b_en1', 'b_sc2', "BUY scientists to fill LQ2 (10)", 72 * hours ),

    ('start', "ec_ws2", "BUILD Workshop #W2", 32 * days),
    ('b_sc2', "ec_lab2", "BUILD Lab #W2", 26 * days),
    ('b_en1', 'ec_lq3', "BUILD Living Quarters #3", 16 * days),
    ('ec_lq3', 'b_sc3', "BUY scientists to fill L1 (10)", 72 * hours),

    (['ec_ws2', 'b_sc3'], 'b_en4', "BUY engineers to fill W2 (36)", 4 * days),

    ('ec_lq3', 'ec_lq4', "BUILD Living Quarters #4", 16 * days),
    ('ec_lq4', 'b_sc4', "BUY scientists to fill L2 (50)", 72 * hours),

    ('ec_ac', 'cap_nav', "CAPTURE Navigator", 0),
    (['**r_lh', 'cap_nav'], 'r_nav', "LAB Navigator", 0),
    ('r_nav', 'r_hwd', "LAB Hyper Wave Decoder", 0),


    ('r_hwd', 'ec_hwd', 'BUILD Hyperwave Decoder', 0),
    ('r_hwd', 'usa', 'B2 [Establish]', 0),
    ('usa', 'usa_gs1', 'B2 BUILD General Stores', 10 * days),
    ('usa', 'usa_lrs', 'B2 BUILD Hyperwave Decoder', 10 * days),
    ('usa_gs1', 'usa_ws1', 'B2 BUILD Workshop 1', 32 * days),
    ('usa_gs1', 'usa_ws2', 'B2 BUILD Workshop 2', 32 * days),
    ('usa_gs1', 'usa_lq1', 'B2 BUILD Living Quarters 1', 16 * days),
    ('usa_lq1', 'usa_lq2', 'B2 BUILD Living Quarters 2', 16 * days),

    (['**r_lh', '**r_hwd'], 'r_hpr', 'LAB Heavy Plasma Rifle', 0),
    ('r_hpr', 'r_hpc', 'LAB Heavy Plasma Clip', 0),

    ('r_hpc', 'r_medi', 'LAB Medikit', 0),
    ('r_medi', 'r_small', 'LAB Small Launcher', 0),
    ('r_small', 'r_sb', 'LAB Stun Bomb', 0),
    ('r_sb', 'r_aa', 'LAB Alien Alloys', 0),
    ('r_aa', 'r_pa', 'LAB Personal Armour', 0),
    ('r_pa', 'r_pb', 'LAB Plasma Beam', 0),
    ('r_pb', 'r_ag', 'LAB Alien Grenade', 0),

    ]


items = [Item(*k) for k in raw_items]


def str_time(t):
    if not t: t = 0
    d = t / days
    h = (t - d*days) / hours
    rem = (t - d*days - h*hours)
    assert rem == 0

    return ('%sd ' % d if d else '') + ('%sh' % h if h else '')


print '''
digraph {
node [shape="record"];

'''

for i in items:
    l = i.descr
    if i.dur:
        l += '|' + str_time(i.dur)
    print '%s [label="%s"];' % (i.name, l)
    after = i.after if isinstance(i.after, list) else [i.after]
    for a in after:
        s = ''
        if a.startswith('**'):
            a = a[2:]
            s = ', style="invisible", arrowhead="none"'
        print '%s -> %s [label=""%s];' % (a, i.name, s)


for group in ['LAB', 'B2']:
    print '''subgraph cluster_%s {''' % group
    print 'style = "invisible";'
    for i in items:
        if i.descr.startswith(group):
            print '%s' % (i.name,)

    print '}'

print '}'
