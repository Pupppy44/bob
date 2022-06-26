<p align="center"> <img width="default" height="300"
        src="https://cdn.discordapp.com/attachments/736359303744585821/990590102843568138/bob.png">
<h3 align="center">A 2D Graphics Library for Windows</h3>
</p>
<h1>Bob</h1>
<p>Bob is a 2D graphics library for Windows 7 and above and for operating systems that support Direct2D. Bob uses
    Direct2D as its renderer hence the OS support, and it's relatively small to add to your project and use.</p>
<h1>Features</h1>
<b>
Objects [Rounded rectangles, circles]<br>
Text<br>
Images [BMP, PNG, JPG, etc?]<br>
Click Events<br>
Headers only, small executables, no DLLs


</b>
<br>
Bob is brand new, so more is anticipated to arrive.
<h1>How to Use</h1>
To use Bob, simply add the <b>bob</b> folder to your project and include <b>bob.h</b>. Make sure you're utilizing a computer where <b>d2d1.lib</b> is available.
<h1>Examples</h1>

```cpp
// Simple example. Easy to set up and get goin'
#include <bob.h>
int main() {
    bob bob;
    bob
        .init("Bob", 640, 480)
        .start();
    return 0;
}
```
```cpp
// Ultimate example. Basically documentation
#include <bob.h>
int main()
{

	bob bob;

	// Set the app's background with RGB
	bob_color bg{ 255,255,255 };
	bob.set_bg(bg);

	bob_rect rect;
	// X, Y, Width, Height
	rect.x = (640 / 2) - 100;
	rect.y = (480 / 2) - 100;
	rect.w = 100;
	rect.h = 100;

	// Set radius of rectangle [0], if it's aliased [true], and RGB color
	rect.radius = 100;
	rect.aliased = false;
	rect.color = { 0,0,0 };

	// Click event 
	rect.add_click([&](int x, int y) {
		// Do what you want with x and y.
		});
	rect.click(0, 0); // Triggers click. Must provide X and Y.

	bob_image img;
	img.x; img.y; img.w; img.h; // Dimensions
	img.path; // Image path, doesn't do anything if invalid
	img.opacity; // Opacity of image
	img.crop; // You can create a crop box for the image. Width and height are required. It's a bob_rect.

	bob_text text;
	text.x; text.y; text.w; text.h; // Dimensions
	text.px; text.font; text.text; // Font size, font name, text content, respectively
	text.font_spacing; text.font_weight; text.color; // Font spacing [1-5], font weight [100-700], RGB color
	text.centered; text.aliased; // Center text [false], aliasing of text [false]
	
	bob
		.init("Bob", 640, 480, true)
		// Title, width, height, load mode, respectively
		// Load mode makes your application a simple rectangle in the center, good for loading screens. [false]
		.add_obj(rect) // Adds the rectangle to the rendering list
		.alert("Alert", "I used Bob, therefore I am cool.") // Alert box. [box title, box content/message]
		.start();
	return 0;
}
```
# That's All
That's all for this file. Don't hesitate to create an issue if you have a problem. bye
