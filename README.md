# Replacement LED bargraph VU & PPM for Revox B77
### "Full RGB" LED display that can show either VU, PPM or VU+PPM simultaneously

I bought myself an old Revox B77 that needed some fixing. Mainly as a fun project, I have always wanted a two track high speed B77. Among other things the left VU meter was broken. The spool winding was broken and despite my best efforts I just couldn't get a hold of the tail end of the extemely thin spool wire to re-attach it. So looking around on the internet for an oem spare part VU meter I found that they have been out of production for a while and the prices have sky rocketed. With shipping and VAT just one meter would cost me $250. And since it was said that, in worst case, they can differ ever so slightly in color and look, there was a small risk that I would need to buy two. So I decided why not have a bit of fun instead designing some pcb with schematics and code, as yet another hobby project!  

All files are here on GitHub for you to order an empty pcb, the front panel (which also is just a regular pcb) and the electronic parts for you to assemble, solder and load the code on to. 
  
## Programming mode, changing display colors, display mode and "screen saver"
  
### Change display colors
First long press the front panel button __once__ and then toggle through all the color schemes with short presses. To save your choice long press again. 
  
### Change display mode
First long press the button __twice__ and then toggle through the below modes with short presses, preferably with music playing so you can differ between the modes. To save your choice long press once again. Display modes available are:
- VU (dot)
- VU (bar)
- PPM (dot)
- PPM (bar)
- VU (bar) and PPM (dot)

### Change "screen saver"
First long press the button __three__ times and then toggle through any available screen savers. After each short press the current choice will be shown for two seconds. The "screen saver OFF" option will just show the normal meter. To save your choice long press once again.
And yeah, it is not a screen saver in the real sense of the word, it's just some fun color schemes that can be activated when there has been no input signal for a while.
  
### Leaving programming mode  

There is a timeout when i programming mode, if no button is pressed at all for 15 seconds the screen will blink once and you will be back in normal mode. If you haven't long pressed to save your current choice any change will be ignored.

## Original vs LED having matching levels and VU ballistics
YouTube video:  
<a href=https://youtu.be/gn2JyQfEoPc><img width="45%" src=https://github.com/user-attachments/assets/523b3b31-b0e6-4c62-a050-417aab0bb54a></a>

## Changing color modes
YouTube video:   
<a href="https://www.youtube.com/watch?v=gmQ4PkJidRk"><img width="45%" src=https://github.com/user-attachments/assets/0d9f4aed-5af1-4f7e-94bf-85054c0e0e47></a>

## Correct frequency response
YouTube videos:  
<a href="https://www.youtube.com/watch?v=C4RWhSTwp5w"><img width="45%" src=https://github.com/user-attachments/assets/98970c9d-ecae-4154-865b-9245905eb742></a> <a href="https://www.youtube.com/watch?v=5C-VhZuq3Lk"><img width="45%" src=https://github.com/user-attachments/assets/f2c46d56-bdae-4124-bc9c-d3463105e655></a>

## Some examples of color and display modes:
  
In the pictures below the display is in program mode (shown by a white LED being turned on in the lower part of left display). Different color schemes are easily added, some distinct and some more oddball color schemes are already in the code.
  
### VU (dot)
<img width="45%" src=https://github.com/user-attachments/assets/a6dfecb9-3bba-47e1-80e3-b6fcf44dc101> <img width="45%" src=https://github.com/user-attachments/assets/e4168efc-3b17-4b72-8f82-e2f3c701a1bc>

### VU (bar) and PPM (dot)
<img width="45%" src=https://github.com/user-attachments/assets/92b40286-546e-4a20-a816-486adcd13276> <img width="45%" src=https://github.com/user-attachments/assets/f705492c-b663-408e-8e03-04f2c20e6a73>

