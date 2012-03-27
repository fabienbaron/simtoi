#!/bin/python

"""
Plots histogram files generated by MultiNestToHistogram.py
"""

from optparse import OptionParser
import re
try:
    import Gnuplot
except:
    print "gnuPlot is required for this script!  Exiting."
    exit()

def plot(infile, outfile, param_num, term, ext, size="5in,3.5in", range_x=[], range_y=[], label_x=""):
    """
    Plots the specified
    """
    
    if len(range_x) == 2:
        range_x = '[' + ':'.join(range_x) + ']'
    else:
        range_x = '[:]'
        
    if len(range_y) == 2:
        range_y = '[' + ':'.join(range_y) + ']'
    else:
        range_y = '[:]'
        
    mult = '1'
    add = '0'
    
    g = Gnuplot.Gnuplot()

    g('reset')
    g('set term ' + term + ' size ' + size)
    g('set size square')
    g('set datafile sep ","')

    center = str(3*param_num + 1)
    width = str(3*param_num + 2)
    count = str(3*param_num + 3)
    
    g('set xrange ' + range_x)
    g('set yrange ' + range_y)
    g('unset ytics')
    g('set output "' + outfile + '.' + ext + '"')
    g('set xlabel "' + label_x + '"')
    #g('set xtics ' + str(xfreq))
    #g('set mxtics 2')
    g('set style histogram columnstacked')
    g('set style fill solid 1.0 noborder')
    # a workaround to ensure the tic marks on on top of the plots:
    g('set grid front')
    g('unset grid')
    g('plot "' + infile +'" using ($' + center + ' * ' + mult + ' + ' + add + '):' + count + ' with boxes notitle"')
    g('unset output')


def main():

    # separation character for files.  If genreated using MultiNestToHistogram.py, it's a comma
    sep = ','

    usage = "Usage: %prog [options] filename"
    parser = OptionParser(usage=usage)
    parser.add_option("--labels", dest="labels", action="store", type="string",
        help="Labels used for X-axis of generated plots")
    parser.add_option("--offsets", dest="offsets", action="store", type="string",
        help="Zero point offsets applied to X-axis data") 
    parser.add_option("--term", dest="term", action="store", type="string", default="postscript enhanced color",
        help="Output terminal (for gnuPlot) [default: '%default']")
    parser.add_option("--ext", dest="ext", action="store", type="string", default="eps",
        help="Extension for output file, use when --term is specified [default: '%default']")
    parser.add_option("--size", dest="size", action="store", type="string", default="5in,3.5in",
        help="Output plot size (for gnuPlot) [default: '%default']") 
           

    (options, args) = parser.parse_args()
    
    if len(args) == 0:
        print "No filename specified, exiting."
        exit()

    # now read the filenames
    filename = args[0]
    base_filename = filename[0:len(filename)-5] # filename.hist, remove 'hist'

    # read the first non-comment line in the histogram to determine 
    # the number of parameters
    n_params = 0
    infile = open(filename)
    for line in infile:
        if not (len(line) > 0) or re.match("^[a-zA-Z#;,\"]", line):
            continue
        
        line = line.strip()
        line = re.split(sep, line)
        n_params = len(line) / 3
        break
    infile.close()
        
    if n_params == 0:
        print "Not enough columns in the input file.  Exiting."
        quit()
        
    for param in range(0, n_params):
        outfile = base_filename + '_' + str(param)
        plot(filename, outfile, param, options.term, options.ext, size=options.size)
        
    
    
    
# Run the main function if this is a top-level script:
if __name__ == "__main__":
    main()
