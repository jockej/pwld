# Maintainer:  <joakim.jalap@gmail.com>
pkgname=pwld-git
pkgver=0.1
pkgrel=1
epoch=
pkgdesc="A daemon for keeping track of different keyboard layouts in X."
arch=('i686' 'x86_64')
url="https://github.com/jockej/pwld"
license=('GPL')
groups=()
depends=('libbsd' 'xorg-server')
makedepends=('git')
checkdepends=()
optdepends=()
provides=('pwld')
conflicts=('pwld')
replaces=()
backup=()
options=()
install=
changelog=
source=()
noextract=()

_gitroot=git://github.com/jockej/pwld.git
_gitname=pwld

build() {
  cd "$srcdir"
  msg "Connecting to GIT server...."

  if [[ -d "$_gitname" ]]; then
    cd "$_gitname" && git pull origin
    msg "The local files are updated."
  else
    git clone "$_gitroot" "$_gitname"
  fi

  msg "GIT checkout done or server timeout"
  msg "Starting build..."

  rm -rf "$srcdir/$_gitname-build"
  git clone "$srcdir/$_gitname" "$srcdir/$_gitname-build"
  cd "$srcdir/$_gitname-build"

  #
  # BUILD HERE
  #
  ./autogen.sh
  ./configure --prefix=/usr
  make
}

package() {
  cd "$srcdir/$_gitname-build"
  make DESTDIR="$pkgdir/" install
}

