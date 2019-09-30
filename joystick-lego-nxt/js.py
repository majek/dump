import nxt
import pygame
import time
import os
import nxt.motor
import datetime

os.environ["SDL_VIDEODRIVER"] = "dummy"


def main():
    print "[+] Looking for joystick"
    while True:
        pygame.init()
        pygame.display.init()
        pygame.display.set_mode((1,1))

        if pygame.joystick.get_count() < 1:
            time.sleep(1)
            continue
        break

    js = pygame.joystick.Joystick(0)
    js.init()


    print "[+] Looking for nxt"
    while True:
        try:
            n = nxt.find_one_brick(host="00:16:53:07:5D:9E")
        except nxt.locator.BrickNotFoundError:
            continue
        break
    n.play_tone(400,345)
    lmotor = nxt.motor.Motor(n, nxt.motor.PORT_A)
    rmotor = nxt.motor.Motor(n, nxt.motor.PORT_C)

    print "[+] Got it!"


    l, r = 0, 0
    while True:
        for event in pygame.event.get():
            l = js.get_axis(1)
            r = js.get_axis(5)
        update(lmotor, rmotor, -l, r)

def adjust(n):
    # -1 ... 0 .... 1
    neg = n < 0
    k = (n*n) * 100
    if neg:
        k = k * -1
    return k

last_l = 0
last_r = 0
last_ts = datetime.datetime.now()

def update(lmotor, rmotor, l, r):
    global last_l, last_r, last_ts

    now = datetime.datetime.now()
    if now - last_ts < datetime.timedelta(milliseconds=90):
        return False

    l = adjust(l)
    r = adjust(r)
    ldiff = abs(last_l-l)
    rdiff = abs(last_r-r)

    if ldiff == 0 and rdiff == 0:
        return False

    if ldiff > rdiff:
        lmotor.run(l)
        last_l = l
        last_ts = now
    else:
        rmotor.run(r)
        last_r = r
        last_ts = now
    print l, r


if __name__ == '__main__':
    main()


