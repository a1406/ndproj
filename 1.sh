find ./ -type f | grep nd | sed 's/^\(.*\)nd\(.*\)/git mv \1nd\2 \1ne\2/g'
git mv ./nd_app ./ne_app
git mv ./nd_cliapp ./ne_cliapp
git mv ./nd_common ./ne_common
git mv ./nd_crypt ./ne_crypt
git mv ./nd_crypt/rsah ./ne_crypt/rsah
git mv ./nd_net ./ne_net
git mv ./nd_quadtree ./ne_quadtree
git mv ./nd_srvcore ./ne_srvcore
cat cscope.files |  sed 's/\(.*\)/sed \-i \'s\/ND_DEBUG\/NE_DEBUG\/g\' \1 /g'
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ND_DEBUG\/NE_DEBUG\/g\' \1 /g"  > 1.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ND_\/NE_\/g\' \1 /g"  > 1.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/NDINT\/NEINT\/g\' \1 /g"  > 1.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/NDUINT\/NEUINT\/g\' \1 /g"  > 1.sh


cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/\\\\([^a-zA-Z]\\\\)nd_\/\\\\1ne_\/g\' \1 /g"  > 1.sh

cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndatomic\/neatomic\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndtimer\/netimer\/g\' \1 /g"  > 2.sh

cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndlbuf_\/nelbuf_\/g\' \1 /g"  > 2.sh

cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndprint\/neprint\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/NDT\/NET\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndchar\/nechar\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndtime_t\/netime_t\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndthread_t\/nethread_t\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndxml\/nexml\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndsocket_t\/nesocket_t\/g\' \1 /g"  > 2.sh

cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndudt_pocket\/neudt_pocket\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndth_handle\/neth_handle\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndudt_header\/neudt_header\/g\' \1 /g"  > 2.sh

cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndudp_packet\/neudp_packet\/g\' \1 /g"  > 2.sh

cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndt_header_size\/net_header_size\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/NDERR_BADPACKET\/NEERR_BADPACKET\/g\' \1 /g"  > 2.sh

cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/NDERR_\/NEERR_\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/NDHANDLE_\/NEHANDLE_\/g\' \1 /g"  > 2.sh
cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/NDUDT_\/NEUDT_\/g\' \1 /g"  > 2.sh

cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/NDNET_MSGENTRY\/NENET_MSGENTRY\/g\' \1 /g"  > 2.sh

cat cscope.files |  sed "s/\(.*\)/sed \-i 's\/ndstr\/nestr\/g\' \1 /g"  > 2.sh

NDNET_MSGENTRY

NDUDT_

ndudt_header
ndprint
NDT
ndchar
ndtime_t
NDTH_FUNC
ndthread_t
ndth_handle
ndxml
ndsocket_t
ndudt_pocket

ndatomic_t
ndtimer_t