#!/bin/bash

# Script for making releases to Source Forge and web site
#   automatically edits version files
#   automatically manages tags/branches
#   generates change logs
#   generates documentation packs
#   generate tarballs/archives
#   uploads to Source Forge
#   uploads to opalvoip web site

if [ "$1" = "-x" ]; then
  shift
  set -x
fi

#
#  set common globals
#

GIT="git"
RSYNC="rsync -e ssh -avz"
TAR=tar
ZIP=zip

SNAPSHOTS=./snapshots
WEB_HOST=files.opalvoip.org
WEB_HTML_DIR="/www/files.opalvoip.org/html"
WEB_DOCS_DIR=${WEB_HTML_DIR}/docs
WEB_CHANGELOG_DIR=${WEB_HTML_DIR}/docs/ChangeLogs

BINARY_EXT=( exe dll lib wav png gif ico sw mdb dsp dsw vcp vcw wpj wsp )

COMMAND_NAMES=(show \
               clean \
               trytag \
               tag \
               log \
               docs \
               arch \
               upsf \
               web \
               all \
              )
FUNCTIONS=(    show_tags \
               clean_copy \
               try_tag \
               create_tag \
               create_changelog \
               create_docs \
               create_archive \
               upload_to_sourceforge \
               update_website \
               do_all_for_release \
              )


#
#  parse command line
#

if [ -z "$1" -o -z "$2" -o -z "$3" ]; then
  echo "usage: $0 cmd(s) proj [ "stable" ] tag [ prevtag ]"
  echo "  cmd may be one of: ${COMMAND_NAMES[*]}"
  echo "  proj may be one of: ptlib opal ipp-codecs"
  echo "  tag is either 'next' or 'last' or an aritrary name"
  exit
fi

COMMANDS=$1
shift

base=$1
shift

if [ -z "$SOURCEFORGE_USERNAME" ]; then
  SOURCEFORGE_USERNAME=`whoami`
fi

case "$base" in
  ptlib | opal)
    repository=ssh://$SOURCEFORGE_USERNAME@git.code.sf.net/p/opalvoip/$base
  ;;
  ipp-codecs)
    repository=ssh://$SOURCEFORGE_USERNAME@git.code.sf.net/p/ippcodecs
  ;;
  *)
    echo Unknown base project: $base
    exit
esac

extract_versions_cmd="$GIT ls-remote ${repository} | awk '/[[:space:]]refs\/tags\/v[0-9]+_[0-9]*["
if [ "$1" = "stable" ]; then
  extract_versions_cmd+=02468
  shift
else
  extract_versions_cmd+=13579
fi

extract_versions_cmd+=']_[0-9]+$/{gsub(/.*\/v/,"");gsub("_"," ");print}'
extract_versions_cmd+="' | sort -k1n -k2n -k3n"

release_tag=$1
previous_tag=$2


echo "Releasing ${base}: \"${COMMANDS[*]}\" $release_tag $previous_tag"

#
# get release tag
#

#echo "Getting previous version: ${extract_versions_cmd}"
release_version=
previous_version=( `/bin/sh -c "${extract_versions_cmd} | tail -1"` )
if [ -z $previous_version ]; then
  echo Could not find last release!
  exit
fi

if [ "$release_tag" = "next" ]; then
  release_version=( ${previous_version[*]} )
  let release_version[2]++
  release_tag=v${release_version[0]}_${release_version[1]}_${release_version[2]}
elif [ "$release_tag" = "last" ]; then
  release_version=( ${previous_version[*]} )
  previous_version=( `/bin/sh -c "$extract_versions_cmd | tail -2 | head -1"` )
  release_tag=v${release_version[0]}_${release_version[1]}_${release_version[2]}
else
  release_version=( `echo $release_tag | awk '/v[0-9]+_[0-9]+_[0-9]+/{gsub("v","");gsub("_"," ");print}'` )
fi

if [ -z $previous_tag ]; then
  previous_tag=v${previous_version[0]}_${previous_version[1]}_${previous_version[2]}
else
  previous_version=( `echo $previous_tag | awk '/v[0-9]+_[0-9]+_[0-9]+/{gsub("v","");gsub("_"," ");print}'` )
fi

release_branch=v${release_version[0]}_${release_version[1]}
exists=`$GIT ls-remote --heads ${repository} | grep "$release_branch" 2>/dev/null`
if [ -z "$exists" ]; then
  echo "Branch ${release_branch} does not exist yet, using master"
  release_branch=master
fi

previous_verstr=${previous_version[0]}.${previous_version[1]}.${previous_version[2]}
release_verstr=${release_version[0]}.${release_version[1]}.${release_version[2]}

echo "Release version: ${release_verstr}, previous: ${previous_verstr}"


#
#  set calculated names
#
SRC_ARCHIVE_TBZ2=${SNAPSHOTS}/${base}-${release_verstr}.tar.bz2
DOC_ARCHIVE_TBZ2_BASE=${base}-${release_verstr}-htmldoc.tar.bz2
DOC_ARCHIVE_TBZ2=${SNAPSHOTS}/${DOC_ARCHIVE_TBZ2_BASE}
BASE_TAR=${base}-${release_verstr}

SRC_ARCHIVE_ZIP=${SNAPSHOTS}/${base}-${release_verstr}-src.zip
DOC_ARCHIVE_ZIP=${SNAPSHOTS}/${base}-${release_verstr}-htmldoc.zip
BASE_ZIP=${base}

SOURCE_FORGE_FILES=( $SRC_ARCHIVE_TBZ2 $DOC_ARCHIVE_TBZ2 $SRC_ARCHIVE_ZIP $DOC_ARCHIVE_ZIP )

CHANGELOG_BASE=ChangeLog-${base}-${release_tag}.txt
CHANGELOG_FILE="${base}/$CHANGELOG_BASE"
VERSION_FILE="${base}/version.h"
REVISION_FILE="${base}/revision.h.in"



#
# show tags
#

function show_tags () {
  echo "Release  tag for $base is $release_tag"
  echo "Previous tag for $base is $previous_tag"
}

#
# check out
#

function clean_copy () {
  exists=`$GIT ls-remote --tags ${repository} | grep "$release_tag" 2>/dev/null`
  if [ -d "$base" ]; then
    echo "Cleaning $base"
    rm -rf "$base/html" "$base/autom4te.cache"
    pushd $base >/dev/null
    $GIT clean -x --force --quiet
    if [ -n "$exists" ]; then
      echo "Switching to $release_tag of $base"
      $GIT checkout --detach ${release_tag}
    else
      echo "Switching to HEAD of $release_branch of $base"
      $GIT checkout ${release_branch}
    fi
    popd >/dev/null
  else
    if [ -n "$exists" ]; then
      echo "Cloning $release_tag of $base"
      $GIT clone --branch ${release_tag} ${repository} "$base"
    else
      echo "Cloning HEAD of $release_branch of $base"
      $GIT clone --branch ${release_branch} ${repository} "$base"
    fi
    pushd $base > /dev/null
    $GIT config user.email "$SOURCEFORGE_USERNAME@sf.net"
    $GIT config user.name "PTLib/OPAL Maintainer"
    popd > /dev/null
  fi
}


#
# switch code to tag or check out of not done yet
#

function switch_to_version () {
  if [ -d $base ]; then
    exists=`$GIT ls-remote --tags ${repository} | grep "$release_tag" 2>/dev/null`
    if [ -n "$exists" ]; then
      echo "Switching to $release_tag of $base"
      ( cd $base ; $GIT checkout --detach ${release_tag} )
    else
      echo "Switching to HEAD of $release_branch of $base"
      ( cd $base ; $GIT checkout $release_branch >/dev/null )
    fi
  else
    clean_copy
  fi
}


#
# create new tag
#

function create_tag () {
  exists=`$GIT ls-remote --tags ${repository} | grep "$release_tag" 2>/dev/null`
  if [ -n "$exists" ]; then
    echo "Tag $release_tag in $base already exists!"
    return
  fi

  if [ -d $base ]; then
    echo "Switching to $release_branch of $base"
    ( cd $base ; $GIT checkout $release_branch >/dev/null )
  else
    clean_copy
  fi

  if [ "$release_branch" = "trunk" -a ${release_version[2]} = 0 ]; then
    release_branch=branches/v${release_version[0]}_${release_version[1]}
    if [ -n "$debug_tagging" ]; then
      echo "Would tag ${repository} ${repository}/$release_branch"
      echo "Would switch to $release_branch of $base"
    else
      echo "Creating branch $release_tag in $base"
      ( cd $base ; $GIT tag $release_branch )
    fi
  fi

  need_push=false
  if [ -n "$release_version" -a -e "$VERSION_FILE" ]; then
    version_file_changed=0
    file_version=`awk '/MAJOR_VERSION/{printf "%s ",$3};/MINOR_VERSION/{printf "%s ",$3};/PATCH_VERSION/{printf "%s",$3}' "$VERSION_FILE"`
    if [ "${release_version[*]}" = "${file_version[*]}" ]; then
      echo "Version file $VERSION_FILE already set to ${release_version[*]}"
    else
      echo "Adjusting $VERSION_FILE from ${file_version[*]} to ${release_version[*]}"
      awk "/MAJOR_VERSION/{print \$1,\$2,\"${release_version[0]}\";next};/MINOR_VERSION/{print \$1,\$2,\"${release_version[1]}\";next};/PATCH_VERSION/{print \$1,\$2,\"${release_version[2]}\";next};/BUILD_TYPE/{print \$1,\$2,\"ReleaseCode\";next};{print}" "$VERSION_FILE" > "$VERSION_FILE.tmp"
      mv -f "$VERSION_FILE.tmp" "$VERSION_FILE"
      version_file_changed=1
    fi

    build_type=`awk '/BUILD_TYPE/{print $3}' "$VERSION_FILE"`
    if [ "$build_type" != "ReleaseCode" ]; then
      echo "Adjusting $VERSION_FILE from $build_type to ReleaseCode"
      awk '/BUILD_TYPE/{print $1,$2,"ReleaseCode";next};{print}' "$VERSION_FILE" > "$VERSION_FILE.tmp"
      mv -f "$VERSION_FILE.tmp" "$VERSION_FILE"
      version_file_changed=1
    fi

    if [ $version_file_changed ]; then
      # Add or remove a space to force check in
      if grep --quiet "revision.h " "$REVISION_FILE" ; then
        sed --in-place  "s/revision.h /revision.h/" "$REVISION_FILE"
      else
        sed --in-place "s/revision.h/revision.h /" "$REVISION_FILE"
      fi
      msg="Update release version number to $release_verstr"
      pushd ${base} > /dev/null
      if [ -n "$debug_tagging" ]; then
        echo $msg
        $GIT diff "`basename $VERSION_FILE`" "`basename $REVISION_FILE`"
        $GIT checkout "`basename $VERSION_FILE`" "`basename $REVISION_FILE`"
      else
        $GIT commit --message "$msg" "`basename $VERSION_FILE`" "`basename $REVISION_FILE`"
        need_push=true
      fi
      popd > /dev/null
    fi
  fi

  if [ -n "$debug_tagging" ]; then
    echo "Would tag ${repository}/$release_branch ${repository}/tags/$release_tag"
  else
    echo "Creating tag $release_tag in $base"
    ( cd $base ; $GIT tag $release_tag )
  fi

  if [ -e "$VERSION_FILE" ]; then
    new_version=( ${release_version[*]} )
    let new_version[2]++
    echo "Adjusting $VERSION_FILE from ${release_version[*]} ReleaseCode to ${new_version[*]} BetaCode"
    awk "/PATCH_VERSION/{print \$1,\$2,\"${new_version[2]}\";next};/BUILD_TYPE/{print \$1,\$2,\"BetaCode\";next};{print}" "$VERSION_FILE" > "$VERSION_FILE.tmp"
    mv -f "$VERSION_FILE.tmp" "$VERSION_FILE"
    ( cd $base && aclocal && autoconf )
    msg="Update version number for beta v${new_version[0]}.${new_version[1]}.${new_version[2]}"
    pushd ${base} > /dev/null
    if [ -n "$debug_tagging" ]; then
      echo $msg
      $GIT diff "`basename $VERSION_FILE`"
      $GIT checkout "`basename $VERSION_FILE`"
    else
      $GIT commit --message "$msg" "`basename $VERSION_FILE`"
      need_push=true
    fi
    popd > /dev/null
  fi

  if $need_push; then
    ( cd $base && $GIT pull --rebase && $GIT push --all && $GIT push --tags )
  fi
}


#
# Test cration of new tag
#

function try_tag () {
  debug_tagging=yes
  echo "Trial tagging, repository will NOT be changed!"
  create_tag
}


#
# create changelog
#

function create_changelog () {
  if [ "$previous_tag" = "nochangelog" ]; then
    echo "Not creating Change Log for $base $release_tag"
    return
  fi

  echo "Creating $CHANGELOG_FILE between tags $release_tag and $previous_tag"
  ( cd $base ; $GIT log ${previous_tag}..${release_tag} ) > $CHANGELOG_FILE
}

#
# create an archive
#

function create_archive () {

  clean_copy

  echo Creating source archive $SRC_ARCHIVE_TBZ2
  rm -f $SRC_ARCHIVE_TBZ2
  if [ "${base}" != "${BASE_TAR}" ]; then
    mv ${base} ${BASE_TAR}
  fi
  exclusions="--exclude=.git*"
  $TAR -cjf $SRC_ARCHIVE_TBZ2 $exclusions ${BASE_TAR} >/dev/null
  if [ "${base}" != "${BASE_TAR}" ]; then
    mv ${BASE_TAR} ${base}
  fi

  echo Creating source archive ${SRC_ARCHIVE_ZIP}
  rm -f $SRC_ARCHIVE_ZIP
  if [ "${base}" != "${BASE_ZIP}" ]; then
    mv ${base} ${BASE_ZIP}
  fi
  exclusions="-x *.git*"
  for ext in ${BINARY_EXT[*]} ; do
    files=`find ${BASE_ZIP} -name \*.${ext}`
    if [ -n "$files" ]; then
      $ZIP -g $SRC_ARCHIVE_ZIP $files >/dev/null 2>&1
      exclusions="$exclusions -x *.$ext"
    fi
  done
  $ZIP -grl $SRC_ARCHIVE_ZIP ${BASE_ZIP} $exclusions >/dev/null
  if [ "${base}" != "${BASE_ZIP}" ]; then
    mv ${BASE_ZIP} ${base}
  fi
}

#
#  create a documentation set
#
function create_docs () {

  switch_to_version

  rm -f $DOC_ARCHIVE_TBZ2 $DOC_ARCHIVE_ZIP

  echo Creating documents...
  (
    export PTLIBDIR=`pwd`/ptlib
    export OPALDIR=`pwd`/opal
    if [ "$base" != "ptlib" ]; then
      pushd $PTLIBDIR
      ./configure --disable-plugins
      popd
    fi

    cd ${base}
    pwd
    rm -rf html
    if ./configure --disable-plugins ; then
      make graphdocs
    fi
  ) > docs.log 2>&1

  if [ -d ${base}/html ]; then
    echo Creating document archive $DOC_ARCHIVE_TBZ2
    ( cd ${base} ; $TAR -cjf ../$DOC_ARCHIVE_TBZ2 html )
    echo Creating document archive $DOC_ARCHIVE_ZIP
    ( cd ${base} ; $ZIP -r ../$DOC_ARCHIVE_ZIP html ) > /dev/null
  else
    echo "Documents not created for $base, check docs.log for reason."
  fi
}

#
#  upload files to SourceForge
#

function upload_to_sourceforge () {
  files=
  for f in ${SOURCE_FORGE_FILES[*]}; do
    if [ -e $f ]; then
      files="$files $f"
    fi
  done
  if [ -z "$files" ]; then
    echo "Cannot find any of ${SOURCE_FORGE_FILES[*]}"
  else
    saved_dir="./source_forge_dir"
    if [ -e "${saved_dir}" ]; then
      upload_dir=`cat ${saved_dir}`
      read -rep "Source Forge sub-directory [${upload_dir}]: "
    else
      read -rep "Source Forge sub-directory: "
    fi
    if [ -n "${REPLY}" ]; then
      upload_dir="${REPLY}"
    fi
    if [ -z "${upload_dir}" ]; then
      echo Not uploading.
    else
      echo "${upload_dir}" > "${saved_dir}"
      upload_path=${SOURCEFORGE_USERNAME},opalvoip@frs.sf.net:/home/frs/project/o/op/opalvoip/`echo ${upload_dir} | sed 's/ /\\\\ /g'`
      echo "Uploading files to Source Forge directory \"${upload_path}\""
      rsync -avP -e ssh $files "${upload_path}"
    fi
  fi
}


#
# Update the web site
#

function update_website () {
  if [ -e "$CHANGELOG_FILE" ]; then
    if [ -d "$WEB_CHANGELOG_DIR" ]; then
      echo "Copying $CHANGELOG_FILE to $WEB_CHANGELOG_DIR"
      cp "$CHANGELOG_FILE" "$WEB_CHANGELOG_DIR"
      already_set=`grep v$release_verstr "$WEB_CHANGELOG_DIR/.htaccess"`
      if [ -z "$already_set" ]; then
        echo "Adding description for v$previous_verstr to v$release_verstr"
        echo "AddDescription \"Changes from v$previous_verstr to v$release_verstr of ${base}\" $CHANGELOG_BASE" >> $WEB_CHANGELOG_DIR/.htaccess
      else
        echo "Description already added for v$release_verstr"
      fi
    else
      echo "Copying $CHANGELOG_FILE to ${WEB_HOST}:$WEB_CHANGELOG_DIR"
      scp "$CHANGELOG_FILE" "${WEB_HOST}:$WEB_CHANGELOG_DIR"

      already_set=`ssh $WEB_HOST "grep v$release_verstr $WEB_CHANGELOG_DIR/.htaccess"`
      if [ -z "$already_set" ]; then
        echo "Adding description for v$previous_verstr to v$release_verstr"
        ssh $WEB_HOST "echo \"AddDescription \"Changes from v$previous_verstr to v$release_verstr of ${base}\" $CHANGELOG_BASE\" >> $WEB_CHANGELOG_DIR/.htaccess"
      else
        echo "Description already added for v$release_verstr"
      fi
    fi
  else
    echo "No $CHANGELOG_FILE, use 'log' command to generate."
  fi

  if [ -e $DOC_ARCHIVE_TBZ2 ]; then
    doc_dir="$WEB_DOCS_DIR/${base}-v${release_version[0]}_${release_version[1]}"
    doc_tar="$WEB_DOCS_DIR/$DOC_ARCHIVE_TBZ2_BASE"
    if [ -d "$WEB_DOCS_DIR" ]; then
      echo "Copying $DOC_ARCHIVE_TBZ2 to $WEB_DOCS_DIR"
      rm -rf "$doc_dir"
      mkdir "$doc_dir"
      $TAR -xjf "$DOC_ARCHIVE_TBZ2" -C "$doc_dir" --strip-components 1
    else
      echo "Copying $DOC_ARCHIVE_TBZ2 to ${WEB_HOST}:$WEB_DOCS_DIR"
      scp "$DOC_ARCHIVE_TBZ2" "${WEB_HOST}:$WEB_DOCS_DIR"
      echo "Creating online document directory ${WEB_HOST}:$doc_dir"
      ssh $WEB_HOST "rm -rf $doc_dir ; mkdir $doc_dir ; $TAR -xjf $doc_tar -C $doc_dir --strip-components 1 ; rm $doc_tar"
    fi
  else
    echo "No $DOC_ARCHIVE_TBZ2, use 'docs' command to generate."
  fi
}


#
# Do all the parts for a "normal" release
#

function do_all_for_release () {
  clean_copy
  create_tag
  create_changelog
  create_archive
  create_docs
  upload_to_sourceforge
  update_website
}

#
# Main loop, execute all commands specified.
#

for CMD in ${COMMANDS[*]} ; do
  i=0
  unknownCommand=1
  for NAME in ${COMMAND_NAMES[*]} ; do
    if [ $CMD = $NAME ]; then
      ${FUNCTIONS[i]}
      unknownCommand=0
    fi
    let i=i+1
  done
  if [ $unknownCommand != 0 ]; then
    echo "Unknown command '$CMD'"
  fi
done


