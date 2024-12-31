# 3ds - Theora video player library

I needed an embedded video player to show a cutscene in a game i'm recreating on the 3DS, I found @oreo639's proof of concept ([https://github.com/oreo639/3ds-theoraplayer](url)) and it turned out to be really helpful

It's not a good library by any means whatsoever, it's a dirty hack/butchering of someone else's work, but for my means and purposes it works. I haven't extensively tested this, I have no idea what it can/can't do, and I formally apologise if this pisses anyone off



## Preparing the footage
###Using a website
So... I did try using ffmpeg to convert my footage into ogv, but there was always some sort of tiny, slight artefacting or frame issue or something (on the video itself) no matter if i used FFMPEG through the terminal, or if I used VLC's converter function. This isn't an AD but out of desperation I used [https://convertio.co/mp4-ogv/](url), I can't guarantee safety, or privacy using that website and using FFMPEG is definetly the better way of doing this

###Using FFMPEG
You can create a compatible video file using the following command:

`ffmpeg -i 'input.ext' -vcodec theora -vf scale=400:-1 -acodec libvorbis -ar 32000 "output.ogv"`

You can change the quality by using the `-q:v` flag. The value can be any interger from 0-10 with 10 being the highest quality.
The old 3ds has limited processing power so I do not recomment using 10, but any value between 0 and 7 should be fine. (4 is recommended)

You can also just set the bitrate manually using the `-b:v` flag. (Somewhere around 500k is recomended)

You can place your generated videos in the `/videos` directory on your sd card, if it doesn't exist, create it.

##Usage
TODO

## Building
### Prerequsites:

[dkp-pacman](https://devkitpro.org/wiki/Getting_Started)

3ds-dev 3ds-libvorbisidec 3ds-libtheora

### Compiling:

Once you complete Prerequsites, run `make`.



## Copyright
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

