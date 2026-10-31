

def spacesaving(monitored, stream):
    for e in stream:
        if e in monitored:
            counter = monitored[e]
            incrementcounter(counter)
        else:
            emin = min(monitored)
            counter = monitored[emin]
            del monitored[emin]
            monitored[e] = counter
            incrementcounter(counter)

def incrementcounter(counter):
    
   
 
