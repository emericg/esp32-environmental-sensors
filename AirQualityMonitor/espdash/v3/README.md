
### Summary

Customizations for the ESP-DASH web interface.

Unfortunately these changes are not particularly straightforward to deploy, and the results are not portable between ESP-DASH releases, so they need to be done manually.

### Changes

Change the title in the file `ESP-DASH/vue-frontend/src/main.js` as you see fit, or like this:
```
document.title = "Air Quality Monitor" + " - " + title;
```

Replace the svg tag at the begining of the file `ESP-DASH/vue-frontend/src/components/NavBar.vue` with the one from the navbar_logo.svg:
```
<router-link class="navbar-item" to="/">
  <svg
    CONTENT FROM navbar_logo.svg
  </svg>
</router-link>
```

### Optional

#### New "informations" page

Add the `ESP-DASH/vue-frontend/src/view/Infos_xxx.vue` page

Edit the `ESP-DASH/vue-frontend/src/router.js` page and add our new view:

```
import Infos from './views/Infos_airqualitymonitor.vue';
```

```
{
  path: '/informations',
  name: 'Informations',
  component: Infos_xxx
},
```

Edit the `ESP-DASH/vue-frontend/components/Navbar.vue` page and add our new view:

```
<router-link class="navbar-item" @click.native="open = false" to="/informations">
  Informations
</router-link>
```

#### Remove "donate" button

Edit the `ESP-DASH/vue-frontend/finalize.js`:
```
<link rel=icon href=/favicon.ico> <title>ESPDash</title>
<script data-name="BMC-Widget" async src="https://cdnjs.buymeacoffee.com/1.0.0/widget.prod.min.js" data-id="6QGVpSj" data-description="Support me on Buy me a coffee!" data-message="You can always support my work by buying me a coffee!" data-color="#FF813F" data-position="right" data-x_margin="24" data-y_margin="24"></script>
```

to:
```
<title>ESPDash</title>
```

### Deploy

As stated on the `ESP-DASH/vue-frontend/README.md`:

```sh
cd ESP-DASH/vue-frontend
npm install
npm run build
```

This will update the `ESP-DASH/src/webpage.h` file.
