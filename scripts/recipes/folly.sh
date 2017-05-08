#!/usr/bin/env bash

source common.sh

if [[ ! -d folly ]]; then
  git clone https://github.com/facebook/folly
  cd "$PKG_DIR/folly" || die "cd fail"
  git checkout cd058f18c0331da9d2a20e5cc4c7d256af490508
fi

if [ ! -d /usr/include/double-conversion ]; then
  if [ ! -d "$PKG_DIR/double-conversion" ]; then
      cd "$PKG_DIR" || die "cd fail"
      git clone https://github.com/google/double-conversion.git
  fi
  cd "$PKG_DIR/double-conversion" || die "cd fail"

  # Workaround double-conversion CMakeLists.txt changes that
  # are incompatible with cmake-2.8
  git checkout ea970f69edacf66bd3cba2892be284b76e9599b0
  cmake . -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"
  make $MAKE_ARGS && make install $MAKE_ARGS

  export LDFLAGS="-L$INSTALL_DIR/lib -ldl $LDFLAGS"
  export CPPFLAGS="-I$INSTALL_DIR/include $CPPFLAGS"
fi

cd "$PKG_DIR/folly/folly/" || die "cd fail"

autoreconf --install
LD_LIBRARY_PATH="$INSTALL_DIR/lib:$LD_LIBRARY_PATH" \
  LD_RUN_PATH="$INSTALL_DIR/lib:$LD_RUN_PATH" \
  ./configure --prefix="$INSTALL_DIR" && \
  make $MAKE_ARGS && make install $MAKE_ARGS
