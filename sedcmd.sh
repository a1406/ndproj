#!/bin/sh
USAGE="Usage: `basename $0` [-v] [-f inputfile] [-t tmpfile]";
VERBOSE=false
TMPFILE=1.tmp

#while getopts t:f:v OPTION ; do
#    case "$OPTION" in
#	f) INFILE="$OPTARG" ;;
#	t) TMPFILE="$OPTARG" ;;
#	v) VERBOSE=true ;;
#	\?) echo "$USAGE" ;
#	exit 1 ;;
#   esac
#done

while read INFILE; do
    echo $INFILE;
sed 's/\([^a-zA-Z_\.\-]\)unsigned int\([^a-zA-Z_\.\-]\)/\1NDUINT32\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^unsigned int\([^a-zA-Z_\.\-]\)/NDUINT32\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)int\([^a-zA-Z_\.\-]\)/\1NDINT32\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^int\([^a-zA-Z_\.\-]\)/NDINT32\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)unsigned char\([^a-zA-Z_\.\-]\)/\1NDUINT8\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^unsigned char\([^a-zA-Z_\.\-]\)/NDUINT8\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)char\([^a-zA-Z_\.\-]\)/\1NDINT8\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^char\([^a-zA-Z_\.\-]\)/NDINT8\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)unsigned short\([^a-zA-Z_\.\-]\)/\1NDUINT16\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^unsigned short\([^a-zA-Z_\.\-]\)/NDUINT16\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)short\([^a-zA-Z_\.\-]\)/\1NDINT16\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^short\([^a-zA-Z_\.\-]\)/NDINT16\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)unsigned long\([^a-zA-Z_\.\-]\)/\1NDUINT32\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^unsigned long\([^a-zA-Z_\.\-]\)/NDUINT32\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)long\([^a-zA-Z_\.\-]\)/\1NDINT32\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^long\([^a-zA-Z_\.\-]\)/NDINT32\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE
done
exit 1;

if [ -z $INFILE ]; then
    echo "$USAGE" ;
    exit 1;
fi

sed 's/\([^a-zA-Z_\.\-]\)unsigned int\([^a-zA-Z_\.\-]\)/\1NDUINT32\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^unsigned int\([^a-zA-Z_\.\-]\)/NDUINT32\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)int\([^a-zA-Z_\.\-]\)/\1NDINT32\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^int\([^a-zA-Z_\.\-]\)/NDINT32\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)unsigned char\([^a-zA-Z_\.\-]\)/\1NDUINT8\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^unsigned char\([^a-zA-Z_\.\-]\)/NDUINT8\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)char\([^a-zA-Z_\.\-]\)/\1NDINT8\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^char\([^a-zA-Z_\.\-]\)/NDINT8\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)unsigned short\([^a-zA-Z_\.\-]\)/\1NDUINT16\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^unsigned short\([^a-zA-Z_\.\-]\)/NDUINT16\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)short\([^a-zA-Z_\.\-]\)/\1NDINT16\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^short\([^a-zA-Z_\.\-]\)/NDINT16\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)unsigned long\([^a-zA-Z_\.\-]\)/\1NDUINT32\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^unsigned long\([^a-zA-Z_\.\-]\)/NDUINT32\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/\([^a-zA-Z_\.\-]\)long\([^a-zA-Z_\.\-]\)/\1NDINT32\2/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE

sed 's/^long\([^a-zA-Z_\.\-]\)/NDINT32\1/g' $INFILE > $TMPFILE
mv $TMPFILE $INFILE


#shift `expr $OPTIND - 1`

#sed 's/\([^a-zA-Z_\.\-]\)int\([^a-zA-Z_\.\-]\)/\1NDINT32\2/g' testsed.txt  > 1.tmp

