A: User connect

1: todo: user connect LS
2: todo: user send ID and PWD to LS
3: todo: LS send ID and PWD to VS
4: todo: LS get user info from DB
5: todo: LS send user info to MS
6: todo: MS response OK to LS to user
7: todo: user send heart info to LS to MS
8: todo: MS response scene info to LS to user

================

B: User disconnect

1: todo Nofity MS user logout
2: todo MS send user info to the LS
3: todo LS save user info to the DB

================

C: User goto another map

1: todo User send move msg to LS to MS
2: todo MS send user info and move msg to LS
3: todo LS send user info to new MS
4: todo new MS response OK to LS
5: todo LS send move OK to the old MS
6: todo LS send move OK to the user

================

D: User attack another user


================

LS: login server
VS: verify server
MS: map server
