If you like contribute with code, fork the project work on other parts then I'm working on.
don't worry about hardest commands to implement, I'm taking care of that.

About adding "new" stuff not part of AmosPro, well we can't know the token numbers, so generally don't do it unless it's has been done in Amos Factory version of Amos Pro. In that case, we can extract the token number from there. We try to 100% compatible with Amos Pro if we can.

There are trade off to be made, speed vs capability, in some cases it better to have speed then having exact behavior. A lot of planar effects can be done in chunky format, sure might need drop few commands but it might worth it. 
My idea is to use retroMode.library as backend for graphics. (Already similar to Amos graphics in many ways.)

If we can improve on some with breaking it we can do that.

It is assumed that code is tested in AMOS pro, or tested by Ascii2Amos, we should not need to check that arguments are 100% correct at run time, if possible do verification in "pass1" before program starts, less CPU overhead that way.

Other ways to contribute, right now I'm not so interested in bug rapports, I know there are stuff that is not perfect, I'm making notes while I'm coding and will look over it all once I'm happy with what I have done.

Writing test cases for Amos Kittens, keep this short, if you do, at most 10 lines, I'm not interested in debugging large complicated programs. If works in small program it should work big programs too.

Do not report bugs on things I have not done yet, I'm not going to do it any quicker, I'm systematically working my way from page by page in user manual.

Other ways to contribute Icons / logos / graphics are accepted.
