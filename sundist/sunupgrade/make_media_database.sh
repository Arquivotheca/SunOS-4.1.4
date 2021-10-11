#!/bin/sh

# @(#)make_media_database.sh   1.1  94/10/31  SMI
# before run this program, 
# (1) mount cdrom to /usr/etc/install/tar
# (2) mkdir upgrade upgrade.tmp

#set -vx

MEDIA_DIR="/usr/etc/install/tar"
#MEDIA_DIR="/cdrom"
RELEASE_DIR="${MEDIA_DIR}/export/exec"
#RELEASE_DIR="./export/exec"
#UPGRADE_DIR="${MEDIA_DIR}/upgrade"
UPGRADE_DIR="./upgrade"
UPGRADE_TMP="./upgrade.tmp"
RELEASE="4_1_3"
KARCH_4="sun4"
KARCH_4C="sun4c"
KARCH_4M="sun4m"

PARAMS=""

#
# exit point for this module: if called return to caller, else return to shell
#

make_file_db_cleanup() {
	if [ ! "${MAKE_INCLUDE}" ]
	then
		exit $1
	else
		exit $1
	fi
}

#
# make_file_db($PACKAGE_NAME, $INCLUDE_FILE_NAME, $PATH_PREFIX)
#

make_file_db_root() {
	echo "$1:"
	tar -tvf $1 | awk '
	/\/$/ {
		printf("d%s ", $1);
		split($2, ids, "/");
		printf("%s %s ", ids[1], ids[2]);
		for(i=3;i<8;i++) printf("%s ", $i);
		len = length($8);
		if(len > 2) printf("%s\n", substr($8,2,len-2));
		else printf("/\n");
	}
	/[0-~]$/ {
		printf("-%s ", $1);
		if(NF == 7) {
			split($2, ids, "/");
			stub = substr(ids[2], 3);
			pref = substr(ids[2], 2, 1);
			printf(" %s  %s ", pref, stub);
			for(i=3;i<7;i++) printf("%s ", $i);
			printf("%s\n", substr($7,2));
		}
		else {
			split($2, ids, "/");
			printf("%s %s ", ids[1], ids[2]);
			for(i=3;i<8;i++) printf("%s ", $i);
			printf("%s\n", substr($8,2));
		}
	}'  > $2.tmp
	sort +7 -u $2.tmp > $2
	rm -f $2.tmp
}

make_file_db_kvm() {
	echo "$1:"
	tar -tvf $1 | awk '
	/\/$/ {
		printf("d%s ", $1);
		split($2, ids, "/");
		printf("%s %s ", ids[1], ids[2]);
		for(i=3;i<8;i++) printf(" %s ", $i);
		len = length($8);
		if(len > 2) printf("/usr/kvm%s\n", substr($8,2,len-2));
		else printf("/usr/kvm\n");
	}
	/[0-~]$/ {
		printf("-%s ", $1);
		if(NF == 7) {
			split($2, ids, "/");
			printf(" %s ", ids[1]);
			stub = substr(ids[2], 3);
			pref = substr(ids[2], 2, 1);
			printf(" %s  %s ", pref, stub);
			for(i=3;i<7;i++) printf(" %s ", $i);
			printf("/usr/kvm%s\n", substr($7,2));
		}
		else {
			split($2, ids, "/");
			printf("%s %s ", ids[1], ids[2]);
			for(i=3;i<8;i++) printf(" %s ", $i);
			printf("/usr/kvm%s\n", substr($8,2));
		}
	}'  > $2.tmp
	sort +7 -u $2.tmp > $2
	rm -f $2.tmp
}

make_file_db_usr() {
	echo "$1:"
	tar -tvf $1 | awk '
	/\/$/ {
		printf("d%s ", $1);
		split($2, ids, "/");
		printf("%s %s ", ids[1], ids[2]);
		for(i=3;i<8;i++) printf(" %s ", $i);
		len = length($8);
		if(len > 2) printf("/usr%s\n", substr($8,2,len-2));
		else printf("/usr\n");
	}
	/[0-~]$/ {
		printf("-%s ", $1);
		if(NF == 7) {
			split($2, ids, "/");
			printf(" %s ", ids[1]);
			stub = substr(ids[2], 3);
			pref = substr(ids[2], 2, 1);
			printf(" %s  %s ", pref, stub);
			for(i=3;i<7;i++) printf(" %s ", $i);
			printf("/usr%s\n", substr($7,2));
		}
		else {
			split($2, ids, "/");
			printf("%s %s ", ids[1], ids[2]);
			for(i=3;i<8;i++) printf(" %s ", $i);
			printf("/usr%s\n", substr($8,2));
		}
	}'  > $2.tmp
	sort +7 -u $2.tmp > $2
	rm -f $2.tmp
}

#
# test each of the globals that matter in this shell for initialization
#

check_make_file_db_globals() {
	if [ ! "${KARCH_4}" ] 
	then
		echo "the kernal architecture is not set"
		make_file_db_cleanup 1
	fi
	
	if [ ! "${RELEASE}" ]
	then
		echo "the release level is not set"
		make_file_db_cleanup 1
	fi
	
	if [ ! "${RELEASE_DIR}" ]
	then
		echo "the media location is not set"
		make_file_db_cleanup 1
	fi
}


#
# 	now for those globals not yet initialized
#

check_make_file_db_globals

MANUAL="${MEDIA_DIR}/export/share/sunos_${RELEASE}/manual"
#MEDIA_FILES_DATABASE="${UPGRADE_DIR}/media_files_database"
#MEDIA_PATH_LIST="${UPGRADE_DIR}/media_paths"
TAR_DIR="${RELEASE_DIR}/sun4_sunos_${RELEASE}"
KVM_DIR="${RELEASE_DIR}/kvm"

PROTO_FILE="${RELEASE_DIR}/proto_root_sunos_${RELEASE}"
SYS_FILE_4="${KVM_DIR}/${KARCH_4}_sunos_${RELEASE}/sys"
KVM_FILE_4="${KVM_DIR}/${KARCH_4}_sunos_${RELEASE}/kvm"
SYS_FILE_4C="${KVM_DIR}/${KARCH_4C}_sunos_${RELEASE}/sys"
KVM_FILE_4C="${KVM_DIR}/${KARCH_4C}_sunos_${RELEASE}/kvm"
SYS_FILE_4M="${KVM_DIR}/${KARCH_4M}_sunos_${RELEASE}/sys"
KVM_FILE_4M="${KVM_DIR}/${KARCH_4M}_sunos_${RELEASE}/kvm"

MEDIA_PACKAGES=`ls "${TAR_DIR}"`
#
# now make the file databases
#

make_file_db_kvm "${KVM_FILE_4}" "${UPGRADE_TMP}/kvm_${KARCH_4}_sunos_${RELEASE}.file_db"
make_file_db_kvm "${KVM_FILE_4C}" "${UPGRADE_TMP}/kvm_${KARCH_4C}_sunos_${RELEASE}.file_db"
make_file_db_kvm "${KVM_FILE_4M}" "${UPGRADE_TMP}/kvm_${KARCH_4M}_sunos_${RELEASE}.file_db"
make_file_db_root "${PROTO_FILE}" "${UPGRADE_TMP}/proto_root_sunos_${RELEASE}.file_db"
make_file_db_kvm "${SYS_FILE_4}" "${UPGRADE_TMP}/sys_${KARCH_4}_sunos_${RELEASE}.file_db"
make_file_db_kvm "${SYS_FILE_4C}" "${UPGRADE_TMP}/sys_${KARCH_4C}_sunos_${RELEASE}.file_db"
make_file_db_kvm "${SYS_FILE_4M}" "${UPGRADE_TMP}/sys_${KARCH_4M}_sunos_${RELEASE}.file_db"

cp "${UPGRADE_TMP}/proto_root_sunos_${RELEASE}.file_db" "${UPGRADE}/root_media_files_database"
awk '{print $9}' "${UPGRADE}/root_media_files_database" > "${UPGRADE}/root_media_paths"

cat "${UPGRADE_TMP}/sys_${KARCH_4}_sunos_${RELEASE}.file_db" >> "${UPGRADE_TMP}/kvm_${KARCH_4}_sunos_${RELEASE}.file_db" 
sort +8 -u "${UPGRADE_TMP}/kvm_${KARCH_4}_sunos_${RELEASE}.file_db" > "${UPGRADE_DIR}/kvm_media_files_database.${KARCH_4}"

cat "${UPGRADE_TMP}/sys_${KARCH_4C}_sunos_${RELEASE}.file_db" >> "${UPGRADE_TMP}/kvm_${KARCH_4C}_sunos_${RELEASE}.file_db" 
sort +8 -u "${UPGRADE_TMP}/kvm_${KARCH_4C}_sunos_${RELEASE}.file_db" > "${UPGRADE_DIR}/kvm_media_files_database.${KARCH_4C}"

cat "${UPGRADE_TMP}/sys_${KARCH_4M}_sunos_${RELEASE}.file_db" >> "${UPGRADE_TMP}/kvm_${KARCH_4M}_sunos_${RELEASE}.file_db" 
sort +8 -u "${UPGRADE_TMP}/kvm_${KARCH_4M}_sunos_${RELEASE}.file_db" > "${UPGRADE_DIR}/kvm_media_files_database.${KARCH_4M}"

rm -f "${UPGRADE_TMP}/sys_${KARCH_4}_sunos_${RELEASE}.file_db"
rm -f "${UPGRADE_TMP}/sys_${KARCH_4C}_sunos_${RELEASE}.file_db"
rm -f "${UPGRADE_TMP}/sys_${KARCH_4M}_sunos_${RELEASE}.file_db"
awk '{print $9}' "${UPGRADE_DIR}/kvm_media_files_database.${KARCH_4}" > "${UPGRADE_DIR}/kvm_media_paths.${KARCH_4}"
awk '{print $9}' "${UPGRADE_DIR}/kvm_media_files_database.${KARCH_4C}" > "${UPGRADE_DIR}/kvm_media_paths.${KARCH_4C}"
awk '{print $9}' "${UPGRADE_DIR}/kvm_media_files_database.${KARCH_4M}" > "${UPGRADE_DIR}/kvm_media_paths.${KARCH_4M}"

for PACKAGE in ${MEDIA_PACKAGES}; do
	make_file_db_usr "${TAR_DIR}/$PACKAGE" "${UPGRADE_TMP}/$PACKAGE.p.file_db" 
done
	make_file_db_usr "${MANUAL}" "${UPGRADE_TMP}/manual.p.file_db"

sort +8 -u ${UPGRADE_TMP}/*.p.file_db > "${UPGRADE_DIR}/usr_media_files_database"
#sort +8 -u ${UPGRADE_TMP}/*.file_db > "${UPGRADE_DIR}/media_files_database.${KARCH_4}"
#sort +8 -u ${UPGRADE_TMP}/*.file_db > "${UPGRADE_DIR}/media_files_database.${KARCH_4C}"
#sort +8 -u ${UPGRADE_TMP}/*.file_db > "${UPGRADE_DIR}/media_files_database.${KARCH_4M}"
awk '{print $9}' "${UPGRADE_DIR}/usr_media_files_database" > "${UPGRADE_DIR}/usr_media_paths"
#awk '{print $9}' "${UPGRADE_DIR}/media_files_database.${KARCH_4}" > "${UPGRADE_DIR}/media_paths.${KARCH_4}"
#awk '{print $9}' "${UPGRADE_DIR}/media_files_database.${KARCH_4C}" > "${UPGRADE_DIR}/media_paths.${KARCH_4C}"
#awk '{print $9}' "${UPGRADE_DIR}/media_files_database.${KARCH_4M}" > "${UPGRADE_DIR}/media_paths.${KARCH_4M}"
