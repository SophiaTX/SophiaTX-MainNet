External plugins
--------------

External/runtime-loadable plugins provide a way for building custom applications with advanced functionality 
on top of blockchain core functionality.

## Key Features

- Dynamically specify external plugins to load
- Plugins are developed as standalone shared libraries
- Automatically load dependent plugins in order
- Plugins can specify commandline arguments and configuration file options
- Separate configuration files for each plugin

## Standard plugins vs External plugins

- Standard plugins are static libraries(.a), external plugins must be shared libraries(.so) + their names must 
  contain suffix "_plugin":

```
add_library( track_and_trace_plugin SHARED
             source files...
           )
```
- They both share the same interface(```plugin<T>, which is derived from abstract_plugin```), but external plugins must provide dll import interface on top of that. Format:

```
extern "C" BOOST_SYMBOL_EXPORT std::shared_ptr<abstract_plugin> get_plugin() {
   return std::make_shared<track_and_trace_plugin>();
}
```

## Configuration
In main configuration file are two parameters, which define list of external plugins:
```
# Directory containing external/runtime-loadable plugins binaries (absolute path or relative to the data-dir/)
external-plugins-dir = "external-plugins"

# External plugin(s) to enable, may be specified multiple times
external_plugin = track_and_trace
```
"external_plugin" parameter contains only plugin name. Binary file of such plugin must be stored in directory specified by "external-plugins-dir" parameter
under following naming convetion:
```
lib<plugin name>_plugin.so -> example: "libtrack_and_trace_plugin.so"
```


Plugin-specific parameters are stored in separate configuration file in subdirectory "configs" 
of "data-dir" parameter under naming convention:
```
<plugin name>_plugin_config.ini -> example: "track_and_trace_plugin_config.ini"
``` 

### Example of data-dir directory structure with external plugin:
Lets say program parameter "data-dir" is set to default value: "sophia_app_data". This is how directory structure would look like:

     sophiatx-binaries                                  # directory with sophiaTX binaries
     ├── sophiatxd                                      # SophiaTX demon
     ├── sophia_app_data                                # data-dir
     │   ├── external-plugins                           # external plugins binaries directory
     │   │   ├── libtrack_and_trace_plugin.so
     │   │   └── ...  
     │   ├── configs                                    # configuration files directory
     │   │   ├── config.ini                             # main config
     │   │   ├── track_and_trace_plugin_config.ini      # track_and_trace plugin config
     │   │   └── ...
     │   ├── blockchain                                 # blockchain directory
     │   │   └── ... 
     │   ├── logs                                       # logs directory
     │   │   └── ...
     │   └── p2p                                        # p2p directory
     │       └── ...  
     └── ...      
     
## Code samples:

External Plugin code template is the same as for standard plugins with two minor differencies 
mentioned in "Standard plugins vs External plugins" section. Standard plugin template can be found [here](https://github.com/SophiaTX/SophiaTX/tree/develop/libraries/plugins/template_plugin).


Example of concrete implementation of external plugin (track_and_trace) can be found [here](https://github.com/SophiaTX/SophiaTX/tree/develop/external_plugins/track_and_trace).

## Build instructions:
External plugin as well as all libraries that are linked must be build with -DBUILD_PIC flag to enable [Position Independent Code](https://en.wikipedia.org/wiki/Position-independent_code).