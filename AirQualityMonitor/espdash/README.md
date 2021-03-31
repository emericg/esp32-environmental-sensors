
### Summary

Customizations for the ESP-DASH web interface.

Unfortunately these changes are not particularly straightforward to deploy, and the results are not portable between ESP-DASH releases.

### Changes

Change the title in the file `ESP-DASH/vue-frontend/src/main.js` as you see fit, or like this:

> document.title = title+" - Air Quality Monitor";

Change the img tag in the file `ESP-DASH/vue-frontend/src/components/NavBar.vue` with a new base64 encoded logo file, like this:

> <img src="INSERT logo.base64 CONTENT HERE" alt="Air Quality Monitor">

### Deploy

As stated on the `ESP-DASH/vue-frontend/README.md`:

> npm install

> npm run build

This will update the `ESP-DASH/src/webpage.h` file.
