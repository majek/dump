#!/usr/bin/env python
import subprocess
import socket
import time
import sys
import glob


gcoverage = [0, 0, 0, 0, 0]


blacklist=sys.argv[1].split()

filenames=[]
for dirname in sys.argv[2].split():
    filenames.extend( glob.glob(dirname + "/*.c.gcov") )
    filenames.extend( glob.glob(dirname + "/*.h.gcov") )
filenames.sort()

for fname in blacklist:
    for fname in [fname, fname+'.gcov']:
        if fname in filenames:
            filenames.remove(fname)

print "%-16s %7s %5s %4s %2s %6s %4s %4s     %s" % ('Name', 'No cov', 'Branches', 'taken', '', 'Lines', 'exec', '', 'Missing')
print "----------------------------------------------------------------------"
for filename in filenames:
    fd = open(filename, 'rb')

    branch_taken = 0
    branch_all = 0
    branch_missed = []

    lines = []
    badlines = 0
    cov_braces = 0
    no_coverage_lines = 0
    for line in fd:
        if line.startswith('call') or line.startswith('function'):
            continue
        if line.startswith('branch'):
            if no_coverage_lines:
                continue
            branch_all += 1
            if line.split()[3].strip() in ['0', 'executed']:
                branch_missed.append(str(lineno))
            else:
                branch_taken += 1
            continue
        if cov_braces:
            cov_braces = max(0, cov_braces + line.count('{') - line.count('}'))
            no_coverage_lines += 1
            #print line.strip()
            continue
        if "no coverage" in line or line.strip().endswith(" never"):
            cov_braces = line.count('{') - line.count('}')
            no_coverage_lines += 1
            #print line.strip()
            continue
        runs, lineno, line = line.split(":",2)
        runs = runs.strip()
        lineno = int(lineno.strip())
        if runs == "-" or lineno==0:
            continue
        if runs[0] == "#":
            badlines +=1
        lines.append( (runs, lineno) )

    rets = []
    first_run, first_lineno, last_lineno = None, None, None
    for runs, lineno in lines:
        if runs != first_run:
            if first_run:
                if first_run[0] == "#":
                    rets.append( (first_lineno, last_lineno or first_lineno) )
            first_run = runs
            first_lineno = lineno
            last_lineno = None
        else:
            last_lineno = lineno
    if first_run and first_run[0] == "#":
        rets.append( (first_lineno, last_lineno or first_lineno) )
    fd.close()
    good, all = (len(lines)-badlines), float(len(lines))
    coverage = (good / (all or 1))*100.0
    branch_cov = (branch_taken/float(branch_all or 1)) * 100
    gcoverage[0] += good
    gcoverage[1] += all
    gcoverage[2] += branch_taken
    gcoverage[3] += branch_all
    gcoverage[4] += no_coverage_lines
    slots = []
    maxdiff = max(map(lambda (a,b):b-a, rets) or [0])
    maxdiff = max(2, maxdiff)
    for a, b in rets:
        s = "%i-%i" % (a,b) if a!=b else '%i' % a
        if b-a == maxdiff: s = "*" + s + "*"
        slots.append(s)
    print "%-20s%3s %4i %4i %3.0f%% %6i %5i  %3.0f%%   %s " % (
            filename.rpartition(".")[0],
            no_coverage_lines if no_coverage_lines > 0 else ' ',
            branch_all, branch_taken, branch_cov,
            all, good, coverage, 
            ', '.join(branch_missed) + ' | ' +
            ', '.join(slots))

print "----------------------------------------------------------------------"
code_cov = (gcoverage[0]/float(gcoverage[1] or 1)) * 100
branch_cov = (gcoverage[2]/float(gcoverage[3] or 1)) * 100
print "%-20s%3i %4i %4i %3.0f%% %6i %5i  %3.0f%%   %s " % ('TOTAL',
        gcoverage[4],
        gcoverage[3], gcoverage[2], branch_cov,
        gcoverage[1], gcoverage[0], code_cov, '')

