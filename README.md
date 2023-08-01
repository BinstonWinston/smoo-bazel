# SMOO Bazel

:warning: Still very work in progress, not yet functional :warning:

Conversion of [CraftyBoss](https://github.com/CraftyBoss)' [SMOO](https://github.com/CraftyBoss/SuperMarioOdysseyOnline) and [Sanae](https://github.com/Sanae6)'s [SMOO server](https://github.com/Sanae6/SmoOnlineServer) to use the [Bazel](https://bazel.build/) build system and [protobuf](https://protobuf.dev/) for packet definitions


## Building
### Prereqs
* [Dazel](https://github.com/nadirizr/dazel) for running Bazel in a docker container

### Building mod
:warning: Will only work for when the server is converted to protobuf, not compatible with the original SMOO since the packet format is different :warning:  
`./build-mod.sh` (needs `sudo` unless your user is in docker user group)

:warning: tarball compressing is still a bit flaky, may need to re-run a couple times for dependencies to get picked up :warning:  
Build output will be available at `./bazel-bin/mod/src/smoo_switch.tar.gz`

The build script can be edited from `//mod/src:smoo_switch` to `//mod/src:smoo_ryujinx` to build for Ryujinx (yuzu build packaging still needs to be added)

### Building server
:warning: Not implemented yet :warning:

## Credits
* [CraftyBoss](https://github.com/CraftyBoss) for SMOO mod
* [Sanae](https://github.com/Sanae6) for SMOO server
