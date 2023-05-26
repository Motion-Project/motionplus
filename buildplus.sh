#!/bin/sh

##############################################################################################
#  Build Script for MotionPlus application.
#  This script is currently only functional for Debian based systems.
#  The following is the overall flow:
#  0.  Validate distribution, user parameters and packages
#  1.  Create a temporary directory and copy in the MotionPlus code.
#  2.  Clean out any working files from the code base copied.
#  3.  Tar up the code and move up to directory parent.
#  4.  Retrieve from git the package rules
#  5.  Change to the applicable branch of package rules and move them to appropriate location.
#  6.  Call the packager application (dpkg-buildpackage) output result to a buildlog file.
#  7.  Move resulting files to the parent of the original source code directory and clean up
##############################################################################################

#########################################################################################
#  Declaration of variables needed
#########################################################################################
DEBUSERNAME=$1
DEBUSEREMAIL=$2
GITBRANCH=master
INSTALLPKG=$4
ARCH=$5
BASEDIR=$(pwd)
DIRNAME=${PWD##*/}
VERSION=""
TARNAME=""
TEMPDIR=""
DEBDATE="$(date -R)"
MISSINGPKG=""
DISTO=$(lsb_release -is)
DISTROVERSION=$(lsb_release -rs)
DISTROMAJOR=`echo $DISTROVERSION | cut -d. -f1`
DISTRONAME=$(lsb_release -cs)
PKGARCH=$(dpkg --print-architecture)

##############################################################################################
#  0.  Validate distribution, user parameters and packages
##############################################################################################

if [ -z "$DISTO" ]; then
  echo "This script is only functional for Debian, Ubuntu and Raspbian"
  exit 1
fi

if [ "$DISTO" != "Ubuntu" ] &&
   [ "$DISTO" != "Debian" ] &&
   [ "$DISTO" != "Raspbian" ] ; then
  echo "This script is only functional for Debian, Ubuntu and Raspbian"
  exit 1
fi

if [ -z "$DEBUSERNAME" ] || [ -z "$DEBUSEREMAIL" ] || [ -z "$GITBRANCH" ]; then
  echo
  echo "Usage:    buildplus.sh name email <optional branch>"
  echo "Name:     Name to use for deb package must not include spaces"
  echo "Email:    Email address to use for deb package"
  echo "Branch:   The git branch name of MotionPlus to build (If none specified, uses master)"
  echo "Install:  Install required packages"
  echo "Arch:     Architecture"
  echo
fi

if [ -z "$INSTALLPKG" ]; then
  INSTALLPKG="N"
fi

if [ -z "$DEBUSERNAME" ]; then
  DEBUSERNAME="AdhocBuild"
fi

if [ -z "$DEBUSEREMAIL" ]; then
  DEBUSEREMAIL="AdhocBuild@nowhere.com"
fi

if [ -z "$GITBRANCH" ]; then
  GITBRANCH="master"
fi

if [ -z "$ARCH" ]; then
  ARCH=$(arch)
fi

PARMS="Using Username: $DEBUSERNAME"
PARMS=$PARMS", User Email: $DEBUSEREMAIL"
PARMS=$PARMS", Git Branch: $GITBRANCH"
PARMS=$PARMS", Install Pkgs: $INSTALLPKG"
PARMS=$PARMS", Arch: $ARCH"
PARMS=$PARMS", PKGARCH: $PKGARCH"

echo
echo $PARMS
echo
sleep 3

#########################################################################################
# Find any packages missing.  (not the best method but functional)
#########################################################################################
if !( dpkg-query -W -f'${Status}' "build-essential" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" build-essential"; fi
if !( dpkg-query -W -f'${Status}' "git" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" git"; fi
if !( dpkg-query -W -f'${Status}' "pkgconf" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" pkgconf"; fi
if !( dpkg-query -W -f'${Status}' "autoconf" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" autoconf"; fi
if !( dpkg-query -W -f'${Status}' "automake" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" automake"; fi
if !( dpkg-query -W -f'${Status}' "libtool" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libtool"; fi
if !( dpkg-query -W -f'${Status}' "libavcodec-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libavcodec-dev" ; fi
if !( dpkg-query -W -f'${Status}' "libavdevice-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libavdevice-dev" ; fi
if !( dpkg-query -W -f'${Status}' "libavformat-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libavformat-dev"; fi
if !( dpkg-query -W -f'${Status}' "libswscale-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libswscale-dev"; fi
if !( dpkg-query -W -f'${Status}' "libjpeg-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libjpeg-dev"; fi
if !( dpkg-query -W -f'${Status}' "libpq-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libpq-dev"; fi
if !( dpkg-query -W -f'${Status}' "libsqlite3-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libsqlite3-dev"; fi
if !( dpkg-query -W -f'${Status}' "dpkg-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" dpkg-dev"; fi
if !( dpkg-query -W -f'${Status}' "debhelper" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" debhelper"; fi
if !( dpkg-query -W -f'${Status}' "dh-autoreconf" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" dh-autoreconf"; fi
if !( dpkg-query -W -f'${Status}' "zlib1g-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" zlib1g-dev"; fi
if !( dpkg-query -W -f'${Status}' "libwebp-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libwebp-dev"; fi
if !( dpkg-query -W -f'${Status}' "libmicrohttpd-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libmicrohttpd-dev"; fi
if !( dpkg-query -W -f'${Status}' "gettext" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" gettext"; fi
if !( dpkg-query -W -f'${Status}' "fakeroot" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" fakeroot"; fi
if !( dpkg-query -W -f'${Status}' "libasound2-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libasound2-dev"; fi
if !( dpkg-query -W -f'${Status}' "libpulse-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libpulse-dev"; fi
if !( dpkg-query -W -f'${Status}' "libfftw3-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libfftw3-dev"; fi
if !( dpkg-query -W -f'${Status}' "libopencv-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libopencv-dev"; fi

if [ "$DISTO" = "Ubuntu" ] && [ "$DISTROMAJOR" -ge "20" ]; then
  if !( dpkg-query -W -f'${Status}' "libmariadb-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libmariadb-dev"; fi
elif [ "$DISTO" = "Ubuntu" ] && [ "$DISTROMAJOR" -ge "17" ]; then
  if !( dpkg-query -W -f'${Status}' "libmariadbclient-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libmariadbclient-dev"; fi
elif [ "$DISTO" = "Debian" ] && [ "$DISTROMAJOR" -ge "10" ]; then
  if !( dpkg-query -W -f'${Status}' "libmariadb-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libmariadb-dev"; fi
elif [ "$DISTO" = "Debian" ] && [ "$DISTROMAJOR" -ge "9" ]; then
  if !( dpkg-query -W -f'${Status}' "libmariadbclient-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libmariadbclient-dev"; fi
elif [ "$DISTO" = "Raspbian" ] ; then
  if !( dpkg-query -W -f'${Status}' "libmariadb-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libmariadb-dev"; fi
else
  if !( dpkg-query -W -f'${Status}' "libmysqlclient-dev" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libmysqlclient-dev"; fi
fi

if [ "$DISTO" = "Raspbian" ] && [ "$DISTROMAJOR" -ge "11" ]; then
  if !( dpkg-query -W -f'${Status}' "libcamera-tools" 2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libcamera-tools"; fi
  if !( dpkg-query -W -f'${Status}' "libcamera-dev"   2>/dev/null | grep -q "ok installed"); then MISSINGPKG=$MISSINGPKG" libcamera-dev"; fi
fi

if [ "$MISSINGPKG" = "" ]; then
  echo "All packages installed"
else
  if [ "$INSTALLPKG" = "Y" ]; then
    sudo apt-get update
    sudo apt-get install -y $MISSINGPKG
  else
    echo "The following packages need to be installed with the following command: sudo apt-get install $MISSINGPKG"
    exit 1
  fi
fi

#########################################################################################
#  1.  Create a temporary directory and copy in the MotionPlus code.
#########################################################################################
  TEMPDIR=$(mktemp -d /tmp/motionplus.XXXXXX)

  if [ -f "src/motionplus.cpp" ] ; then
    echo "Using local source code version"
    mkdir $TEMPDIR/motionplus
    cp -r $BASEDIR/. $TEMPDIR/motionplus/
  else
    cd $TEMPDIR
    git clone https://github.com/Motion-Project/motionplus.git
  fi

  cd $TEMPDIR/motionplus
  if ! git checkout $GITBRANCH ; then
    echo Unknown branch
    rm -rf $TEMPDIR
    exit 1
  fi

  cd $BASEDIR
  if [ -f "plus01/motionplus.postinst" ]; then
    echo "Using local package version"
    mkdir $TEMPDIR/motion-packaging
    cp -r $BASEDIR/* $TEMPDIR/motion-packaging
  else
    cd $TEMPDIR
    git clone https://github.com/Motion-Project/motion-packaging.git
  fi

  cd $TEMPDIR/motionplus

#########################################################################################
#  2.  Clean out any working files from the code base copied.
#########################################################################################
  rm -f config.status config.log config.cache Makefile motionplus.service
  rm -f camera1-dist.conf camera2-dist.conf camera3-dist.conf sound1-dist.conf motionplus-dist.conf
  rm -rf autom4te.cache config.h 
  rm -f *.gz *.o *.m4 *.*~
  if [ -d ".github" ]; then
    git rm -rf .github
  fi

#########################################################################################
#  3.  Tar up the code and move up to directory parent.
#########################################################################################
  VERSION=$(scripts/version.sh)
  echo "Version: $VERSION"
  TARNAME=motionplus_$VERSION.orig.tar.gz

  tar --exclude=".*" -zcf $TARNAME *

  mv $TARNAME $TEMPDIR/$TARNAME

  cd ..

#########################################################################################
#  4.  Retrieve from git the package rules
#########################################################################################

  cd $TEMPDIR/motion-packaging

  if [ "$DISTO" = "Ubuntu" ]; then
    cp -rf $TEMPDIR/motion-packaging/plus02 $TEMPDIR/motionplus/debian
  elif [ "$DISTO" = "Debian" ]; then
    cp -rf $TEMPDIR/motion-packaging/plus02 $TEMPDIR/motionplus/debian
  elif [ "$DISTO" = "Raspbian" ] && [ "$DISTROMAJOR" -ge "11" ]; then
    cp -rf $TEMPDIR/motion-packaging/plus03 $TEMPDIR/motionplus/debian
  else
    echo "Unsupported Distribution: $DISTO"
    rm -rf $TEMPDIR
    exit 1
  fi

#########################################################################################
#  4a.  Update the packaging changelog
#########################################################################################
  cd $TEMPDIR/motionplus
  printf "motionplus ($VERSION-1) $DISTRONAME; urgency=medium\n\n  * See changelog in source\n\n -- $DEBUSERNAME <$DEBUSEREMAIL>  $DEBDATE\n" >./debian/changelog

#########################################################################################
#  6.  Call the packager application (dpkg-buildpackage) output result to a buildlog file.
#########################################################################################
  if ! [ $? -eq 0 ]; then
    echo "Unspecified error"
    echo rm -rf $TEMPDIR
    exit 1
  fi
  echo "Building package...."

  dpkg-buildpackage -us -uc -j4 >$TEMPDIR/motionplus_$VERSION-buildlog-$ARCH.txt 2>&1

##############################################################################################
#  7.  Move resulting files to the parent of the original source code directory and clean up
##############################################################################################

  CHK="N"
  if ls $TEMPDIR/motionplus_$VERSION*.deb 1> /dev/null 2>&1; then
    CHK="Y"
  fi

  cd $BASEDIR
  mv $TEMPDIR/motionplus_$VERSION* $BASEDIR
  rm -rf $TEMPDIR
  for FILE in $BASEDIR/motionplus_$VERSION*; do
    NEWNAME="_${FILE##*/}"
    if [ "$DISTO" = "Raspbian" ] ; then
      NEWNAME=pi_$DISTRONAME$NEWNAME
    else
      NEWNAME=$DISTRONAME$NEWNAME
    fi
    mv "$FILE" "$NEWNAME"
  done
#########################################################################################
  if [ $CHK = "Y" ]; then
    echo "The deb packages and build logs have been created and"
    echo "saved in $BASEDIR"
    exit 0
  else
    echo "Build Error.  Check build log "
    echo "saved in $BASEDIR"
    exit 1
  fi
##############################################################################################
