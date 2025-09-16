# VTFLib - A Valve VTF and VMT image format programming library.

VTFLib is a LGPL open source programming library that provides a C and C++ API that, with a few simple functions, can open and save .vtf and .vmt files, providing access to all known features. The library functions independent of Steam, allowing third party applications to use the library without Steam present or running on the target system.

VTFLib includes two GPL example applications, VTFCmd and VTFEdit. VTFCmd is a C command line frontend for VTFLib that can create .vtf and .vmt files from various source formats. It is similar in functionality to Valve's vtex Source SDK utility, but offers a lot more control. VTFEdit is a C++ .NET graphical frontend for VTFLib with viewing and creation capabilities. Both VTFCmd and VTFEdit support several source image formats, including, but not limited to .bmp, .dds, .gif, .jpg, .png and .tga.

## Library/Author Information

* **Title**: VTFLib
* **Written In**: C/C++
* **Date**: July 25th, 2011
* **Authors**: [IAmKnotMax](https://github.com/WereTech), [Skyrym](https://github.com/Sky-rym), [misyltoad](https://github.com/misyltoad), [Neil 'Jed' Jedrzejewski](https://github.com/NeilJed) & [Ryan Gregg](https://nemstools.github.io/)

## Project Structure

The library contains five folders:

* **Bin** - Contains library and example program binaries.
* **Lib** - Contains library C and C++ Header and Inline Files.
* **Sln** - Contains Visual Studio solutions.
* **VTFCmd** - Contains C example program source code.
* **VTFEdit** - Contains C++ .NET example program source code.
* **VTFLib** - Contains C++ library source code.

The project files are for Visual Studio .NET 2003 and 2005; no .NET extensions are used except in VTFEdit. Visual Studio 6.0 project files have also been
included.

## How to Build

### Prerequisites
Visual Studio 2019 or Visual Studio 2022

"Desktop development with C++" workload
#

Open the solution in /VTFEdit-Reloaded/sln/ with Visual Studio 2019 or 2022. Set the build from Debug to Release, then right-click on VTFEdit in the Solution Explorer and click Set as startup project, then right-click VTFEdit again and click Build.

After the Build is finished, the executable will be in /VTFEdit-Reloaded/sln/vs2019/Build/

Move the executable and all dll files to any folder you want VTFEdit-Reloaded to be in, it does not matter.

#
HLLib can be obtained from here, with the dll and the source code if you want to build it yourself. [>>Link](https://web.archive.org/web/20171114194253/http://nemesis.thewavelength.net/files/files/hllib246.zip) 

DevIL.dll from this zip (This may be changed if the project has a working 32 bit compile) [>>Link](https://sourceforge.net/projects/openil/files/DevIL%20Win32%20and%20Win64/DevIL-EndUser-x64-1.8.0.zip/download?use_mirror=phoenixnap)
#

## VTFCmd Usage
Parameters:
```
 -file <path>             (Input file path.)
 -folder <path>           (Input directory search string.)
 -output <path>           (Output directory.)
 -prefix <string>         (Output file prefix.)
 -postfix <string>        (Output file postfix.)
 -version <string>         (Output version.)
 -format <string>          (Output format to use on non-alpha textures.)
 -alphaformat <string>     (Output format to use on alpha textures.)
 -flag <string>           (Output flags to set.)
 -resize                  (Resize the input to a power of 2.)
 -rmethod <string>        (Resize method to use.)
 -rfilter <string>        (Resize filter to use.)
 -rwidth <integer>        (Resize to specific width.)
 -rheight <integer>       (Resize to specific height.)
 -rclampwidth <integer>   (Maximum width to resize to.)
 -rclampheight <integer>  (Maximum height to resize to.)
 -alphathreshold <integer> (Alpha threshold for One Bit Alpha. Pixel alpha below this value is set to 0)
 -gamma                   (Gamma correct image.)
 -gcorrection <single>    (Gamma correction to use.)
 -nomipmaps               (Don't generate mipmaps.)
 -mfilter <string>        (Mipmap filter to use.)
 -nwrap                   (Wrap the normal map for tiled textures.)
 -bumpscale <single>      (Engine bump mapping scale to use.)
 -nothumbnail             (Don't generate thumbnail image.)
 -noreflectivity          (Don't calculate reflectivity.)
 -shader <string>         (Create a material for the texture.)
 -param <string> <string> (Add a parameter to the material.)
 -recurse                 (Process directories recursively.)
 -exportformat <string>   (Convert VTF files to the format of this extension.)
 -silent                  (Silent mode.)
 -pause                   (Pause when done.)
 -help                    (Display vtfcmd help.)
```
Example vtfcmd usage:
```
vtfcmd.exe -file "C:\texture1.bmp" -file "C:\texture2.bmp" -format "dxt1"
vtfcmd.exe -folder "C:\input\*.tga" -output "C:\output" -recurse -pause
vtfcmd.exe -folder "C:\output\*.vtf" -output "C:\input" -exportformat "jpg"
```
## VTFEdit Reloaded Changelog
<details>
	
  v2.1.7
  - Warning and Notification Sounds are now enabled by default.
  - Added button to Help menu to pull up latest GitHub version in browser.
  - Fixed ordering of resize algorithms and exposed more options to user.
  - Upgraded [stb_image_resize.h](https://github.com/nothings/stb/blob/master/deprecated/stb_image_resize.h) to [stb_image_resize2.h](https://github.com/nothings/stb/blob/master/stb_image_resize2.h), VTF creation is much faster now.
  - Mipmaps are now generated with STB2.
  - Upgraded Compressonator to fork [Yellow-Dog-Man/compressonator](https://github.com/Yellow-Dog-Man/compressonator).
	
  v2.1.6
  - Fixed multi-line comments in VMTs not being allowed to save or load.
	
  v2.1.5
  - Fixed reflectivity not being set correctly when saving a VTF.
	
  v2.1.4
  - Fixed mipmap generation still attempting to make mipmaps when it was disabled.
	
  v2.1.3
  - Disables mipmap generation with DXT formats that use Multiple of Four resizing as they do not work as expected.
	
  v2.1.2
  - Fix alpha detection method for batch converting.
	
  v2.1.1
  - Added an option to set a custom alpha background for images that have alpha. Both settings are in the Options tab at the top.
  - Fix DXT1 with One Bit Alpha format not being imported correctly or being functional in source games.
    - Instead, this format will be turned into DXT1 with the One Bit Alpha flag being enabled for it.
  - Added a new Advanced option to set the alpha threshold when creating one bit textures.
    - This will set pixels that have a lower numeric alpha value lower than this number to be 0, and any equal to or higher to be 255.
    - This option is only relevant to DXT1 with One Bit Alpha.
  - Added a parameter for VTFCmd for the alpha threshold when creating one bit textures.
  - Removed HLLib as this is unused after the removal of WAD convert. This removes the requirement of needing msvcr100.dll to run the program.
	
  v2.1.0
  - Fix batch create not setting the shader parameter to what you set for all vmts.
  - Fix an image format placement in code.
  - Fix One Bit Alpha flag not being kept on save.
  - Moved all system sounds to only being heard when both warning popups and notification sounds are enabled.
  - Changed the method that is used to determine whether an image should use Color or Alpha format.
  - It will now iterate through all pixels in the image and check their opacity. Previously this only looked at certain attributes of the image.

  v2.0.9
  - Releases now include VTFCmd.exe (Untested.)
  - Added a saved parameter for the zoom level of the VMT editor (Ctrl+MWheel changes the zoom level for this).
  - Added a color panel that is used to view the representation of the reflectivity value and can be used to change that value when clicking on it.
  - Added option to enable/disable notification sounds.
  - Added option to enable/disable pop-up warnings when an error occurs or closing an opened file.
  - Added a total percentage label to the Batch Convert window.
  - Changed the default text when creating a new VMT to include a line for $basetexture.
  - Changed the image position for VTF files to be centered on the middle panel and scaled to a more appropriate size by default.
  - Optimized some functions.
  - Removed the tabs in favor of adding a panel to the right, as a lot of redundant information was removed.
  - Removed the file system ui elements.
  - Removed WAD convert as it's broken. Don't know what happened to break it.
  - Fixed some conditional statements.
	
  v2.0.8
  - Fixed various issues with UI positioning and other functions not working.
  - Added a few more check boxes for VMT Create dialog.
	
  v2.0.7
  - Adjusted the Convert Folder/Convert WAD dialogs to allow for adjusting the created VMT file instead of defaulting to a bare-minimum file. There is a new button for this on the bottom-left next to the button that was used for only changing VTF options.
  - Forgot to add VTF flags functionality to WAD convert, this is added now.
	
  v2.0.6
  - Added an Edit Resources button for VTF files that are version 7.3 and above.
  - Added a Flags tab to the VTF import dialog. This also works when using the convert folder option and setting them within the Options button on the bottom-left of the dialog.
	
  v2.0.5
  - While importing an image, you can now select "Nearest Multiple of 4", which is the suggested bare-minimum for mipmaps to be generated properly.
  - If you have an image opened, you can now change the version by selecting the new one in the dropdown box in the Info tab, then Save.
  	- Keep in mind this will change the VTF size. So if you are using it for a spray decal, it could possibly go outside of the 512kb limitation when increasing the version.
  - The program's settings file is now contained in the same location that the program is in, instead of your Roaming folder.
  - The default version has also been changed to 7.4 as mostly commonly that will be the version used by source engine modders.
  - The context menu's options for VTF files has been slightly changed for more clarity.
  - The File menu has been slightly changed for more clarity and better positioning. Creating either VTF or VMT is now under the first option "Create New..."
  - Pulled request by goodusername123, which excludes the sRGB flag from VTFs. This is hopefully a temporary measure.
  - Reverted the syntax highlighter as it's terrible with performance and breaks some functionality. All related code is commented out if the issues can be solved later.
  - Added a way to change the framerate for animated playback (in milliseconds).
  - Fixed the import's resize filtering to use what you chose instead of being forced to use only one type of filter.
  - Adjusted the About dialog to include @WereTech and @Sky-rym as authors.

  v2.0.4
  - Changed the UI to use a better version of the tool bar and added more buttons for ease of access.
  - Adjusted several UI elements to stop text wrapping, causing the rest of the text to not be visible at all.
  - Adjusted some windows to now use a minimum window size, as this was not set before and you were able to resize these windows to be extremely small which also hid controls.
  - Added a Close option to both the File dropdown, and the new tool bar. The shortcut for this is Ctrl + Q. This will close the current file, which has a confirmation prompt so you don't close your file accidentally.
  - Changed the right-click context menu for VTFs to additionally show when clicking within the image window instead of needing to right-click on the image itself.
  - Changed the functionality of the Frame, Face, Slice, and Mipmap numeric bars to only be interactable if any of these properties have more than one to them. (in this case 0 = 1 because C++ starts at 0)
  - Adjusted some VTF flag names.

  v2.0.3
  - Updated the Shaders and Surface Properties list in VMT Create in accordance to the Valve Developer Wiki's current list
  - Added Drag and Drop functionality for images that can be imported normally through the File > Import dialog
	  - Note that if any window is above VTFEdit-Reloaded, or any part of the window is outside monitor bounds, the Import settings will default to the top-left of the screen. I do not have the knowledge to change this behavior myself unfortunately.
  - Changed the ReadOnly property in the Convert Folder and Convert WAD dialog, so you can manually insert or change the path to your folder instead of needing to use the Folder Selector button on the right.
  - Changed the resource files to use "winres.h" and "winver.h" instead of "afxres.h". Supposedly afxres.h is outdated, and has been replaced with the two other headers. So far I haven't noticed any issues with this.

  v2.0.2
  - Updated Compressonator to latest version as of 8/8/22.
  - Added 32-bit version.

  v2.0.1
  - Updated to HLLib v2.4.6.
  
  v2.0.0
  - Migrated to using AMD Compressonator for DXT compression.
  - Fix the green tinge when exporting textures
  - Respect sRGB-ness when generating mipmaps and resizing.
  - Ported to using the latest CLR and VS 2019.
  - Removed crufty sharpening filters (no longer needed with proper sRGB)
  - Use monospace font for VMT editor
  - High DPI support
  - Syntax highlighting support for VMTs
  - Fixed cubemap previews
    - Previously they were doing integer maths on what should be floats
    - Replaced overly complex controls with a simple exposure slider
    - Very simple Reinhard tonemapper for previewing exposure
</details>

## Program Copyright-Permissions

See the lgpl.txt (VTFLib) and gpl.txt (VTFCmd & VTFEdit) files contained in the distribution.
