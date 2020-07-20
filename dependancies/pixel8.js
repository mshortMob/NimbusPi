/*
Copyright (c) 2013 Rohit Vighne
License: The MIT License (MIT)
*/

function pixel8(image, x, y, w, h) {
	//"use strict";

	// Image must be an image, canvas, or video
	// For videos: Pixel data of the currently displayed frame will be extracted
	// For canvases: Why are you using this?
	if (!image.tagName || image.tagName !== "IMG" && image.tagName !== "VIDEO" && image.tagName !== "CANVAS") {
		throw new TypeError("first argument must be image, video, or canvas context.");
	}

	// Defaults for x-offset, y-offset, width, and height values
	if (typeof x !== 'number') x = 0;
	if (typeof y !== 'number') y = 0;
	if (typeof w !== 'number') w = image.width;
	if (typeof h !== 'number') h = image.height;

	// For our friend Internet Explorer, FlashCanvas is supported
	// ExplorerCanvas does not support the getImageData function
	//canvasp8 = document.createElement('canvas');
	var canvasp8=document.getElementById('canvasp8')
	canvasp8.width=w
	canvasp8.height=h
	if (canvasp8.getContext) var ctx = canvasp8.getContext('2d');
	else return;

	// Draw the image/canvas/video and return a CanvasPixelArray of pixel data
	// Image must be from the same origin! Use a server-side proxy if you need cross-domain resources.
	// Like this one: http://benalman.com/projects/php-simple-proxy/
	// See https://developer.mozilla.org/en-US/docs/HTML/Canvas/Pixel_manipulation_with_canvas
	// to find out how to get specific data from the array
	// Or just use the pixel8-provided methods below
	ctx.drawImage(image, x, y, w, h);
	var _data = ctx.getImageData(0, 0, w, h)
	var data = _data.data;
	data.width = _data.width;
	data.height = _data.height;
	
	// Returns {red, green, blue, alpha} object of a single specified pixel
	// or sets the specified pixel.
	data.pixelAt = function (x, y, set) {
		var i = y * this.width * 4 + x * 4;

		if (set) {
			this[i] = set.red;
			this[i + 1] = set.green;
			this[i + 2] = set.blue;
			this[i + 3] = set.alpha;
		} else return {
			red: this[i],
			green: this[i + 1],
			blue: this[i + 2],
			alpha: this[i + 3]
		};
	};

	// Draws the pixel data into a canvas
	data.draw = function(ctx, x, y) {
		ctx.putImageData(_data, x, y);
	};

	return data;
}

/*
Example usage:
	var image = document.getElementById('myimg');
	var data = pixel8(image);
	var pixel = data.pixelAt(0, 0);
	alert("The transparency of the first pixel is: " + pixel.alpha);
*/