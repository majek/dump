#!./venv/bin/python
import time

import nxt.locator
from nxt.motor import *
from nxt.sensor import *

#import nxt.usbsock
#b = nxt.usbsock.find_bricks().next().connect()
#print b.get_battery_level()

# import nxt.locator
# nxt.locator.make_config()


def spin_around(b):
    m_left = Motor(b, PORT_A)
    m_left.turn(100, 360)
    m_right = Motor(b, PORT_A)
    m_right.turn(-100, 360)


brick = nxt.locator.find_one_brick()
print "Battery: %.3f V" % (brick.get_battery_level() / 1000.0, )


light = Light(brick, PORT_2)
m_right = Motor(brick, PORT_A)
m_left = Motor(brick, PORT_B)
m_rotate = Motor(brick, PORT_B)

#motors = SynchronizedMotors(m_left, m_right, 1.)

light.set_illuminated(True)
time.sleep(1)

# 900 --> -900

try:
    while True:
        t = m_rotate.get_tacho().tacho_count
        print "tacho: %s " % (t,)
        if t <= 800:
            d = 1
        else:
            d = -1
        m_rotate.run(d * 100, True)
        while 1:
            t = m_rotate.get_tacho().tacho_count
            l = light.get_lightness()
            print "tacho: %s  light: %s" % (t,l)
            if (t > 800 and d == 1) or (t < -800 and d == -1): break
            time.sleep(0.02)
        print "tacho: %s " % (m_rotate.get_tacho().tacho_count,)
        m_rotate.idle()
        m_rotate.brake()
        print "tacho: %s " % (m_rotate.get_tacho().tacho_count,)


finally:
    time.sleep(0.1)
    m_rotate.idle()
    m_rotate.brake()
    light.set_illuminated(False)
    time.sleep(0.1)
