
### Summary

Customizations for the ESP-DASH web interface.

Unfortunately these changes are not particularly straightforward to deploy, and the results are not portable between ESP-DASH releases, so they need to be done manually.

### Changes

Change the title in the file `ESP-DASH/vue-frontend/src/main.js` as you see fit, or like this:
```
document.title = "HiGrow" + " - " + title;
```

Replace the svg tag at the begining of the file `ESP-DASH/vue-frontend/src/components/NavBar.vue` with the one from the navbar_logo.svg:
```
<router-link class="navbar-item" to="/">
  <svg
    CONTENT FROM navbar_logo.svg
  </svg>
</router-link>
```

### Deploy

As stated on the `ESP-DASH/vue-frontend/README.md`:

```sh
cd ESP-DASH/vue-frontend
npm install
npm run build
```

This will update the `ESP-DASH/src/webpage.h` file.
