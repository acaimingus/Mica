# Packaging

This file describes how to create packages for the MicaListener and the MicaApp applications.

## MicaApp

As the Mica Android app is just an Android Studio project, it can be easily built using Android Studio. This is done by selecting the options under `Build`. From there the wizard can be followed to create an APK.

For release builds an Android keystore is needed. All builds until now have been done using my personal keystore. If you wish to make your own release build, you will have to create a new one.

## MicaListener

### Debian/Ubuntu (.deb)

This section assumes that you are in the directory `Packaging/Debian` in the git repository.

#### Directory structure

A Debian package essentially mirrors the directory structure of the target system. Important files and paths for the project:

* **`DEBIAN/control`**: The core of the package. Contains metadata such as package name, version, architecture, and dependencies.
* **`DEBIAN/postinst`**: A shell script executed after installation (e.g., to reload or enable systemd services).
* **`DEBIAN/prerm`**: A script executed before uninstallation (e.g., to cleanly stop services before deleting files).
* **`usr/bin/`**: The target destination for the executable binaries.
* **`usr/lib/systemd/user/micalistener.service`**: The systemd service configuration for the listener.

#### Updating the package information

As the first step, the package information should be adjusted. This is done in the file `DEBIAN/control`. Please adjust the version number to match the GitHub version. Feel free to add yourself to the list of maintainers if you wish. Verify if the dependencies are still correct and adjust them if that is not the case.

#### Preparing the executable

Compile the C++ project under `../../MicaListener` using CMake. It will generate an executable file called `MicaListener`. Copy that file into `usr/bin/` (**in the packaging directory, not your `/usr/bin`!!!**). Doing this will make the apt package manager install the MicaListener executable under `/usr/bin/`, which in turn means that MicaListener will be in PATH and available through the terminal. Make sure to delete the `dummy.txt` file in the directory, it is only there so git keeps the folder structure.

#### File permissions

Debian packages are strict about file permissions. The control scripts and the executable files must be marked as executable.

```bash
chmod 755 DEBIAN/postinst
chmod 755 DEBIAN/prerm
chmod 755 usr/bin/*
```

#### Building the package

Once all files are in place and the metadata in the `control` file is correct (especially the version number!), you can build the actual package.

Navigate one directory up and use `dpkg-deb`:

```bash
dpkg-deb --build Debian MicaListener_X.Y.Z_amd64.deb
```

This command packages the contents of the `Debian/` folder into the final file `MicaListener_X.Y.Z_amd64.deb`. Replace `X.Y.Z` with your actual version string.

The output `.deb` file is ready to be shared then.

#### Testing

To test the finished package on your system, you can install it as follows:

```bash
sudo apt install ./MicaListener_X.Y.Z_amd64.deb
```

If something isn't right, you can remove it at any time (the name corresponds to the `Package` field in the `control` file):

```bash
sudo apt remove micalistener
```

### No packaging system

If there is no packaging system available or the user does not wish to use one, then it is also possible to set up the listener manually in a few steps.

#### Copying the listener executable

Compile the C++ project under `../../MicaListener` using CMake. It will generate an executable file called `MicaListener`. Copy that file into your `/usr/bin/` folder to have the executable in your path or into a folder of your choosing. Make sure to add that folder to PATH then.

#### Setting up the systemd service

For the automatic starting of the service a systemd service file will have to be written. This has to be adjusted depending on the system, but as a reference the repository `micalistener.service` file can be used. The file has to be put `/usr/lib/systemd/user`. After copying, reload the systemd manager configuration and start the service:

```bash
systemctl --user daemon-reload
systemctl --user enable --now micalistener.service
```

After that, the service should be setup and the Android app can be used.