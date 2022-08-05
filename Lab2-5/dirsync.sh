#!/bin/bash

function ScanDst()
{
    DstPos=$2"/*"
    for FileInDst in $DstPos
        do
            FileInSrc="${FileInDst/#$2/$1}"
            if [[ -d "$FileInDst" ]]
            then
                if [[ -d "$FileInSrc" ]]
                then
                    ScanDst "$FileInSrc" "$FileInDst"
                else
                    rm -rf $FileInDst
                fi
            else
                if [[ !(-e "$FileInSrc") || (-d "$FileInSrc") ]]
                then
                    rm -rf $FileInDst
                elif [[ $FileInDst -ot $FileInSrc ]]
                then
                    cp -fp $FileInSrc $FileInDst
                fi
            fi
        done
}

function ScanSrc()
{
    SrcPos=$1"/*"
    for FileInSrc in $SrcPos
        do
            FileInDst="${FileInSrc/#$1/$2}"
            if [[ -d "$FileInSrc" ]]
            then
                if [[ -d "$FileInDst" ]]
                then
                    ScanSrc "$FileInSrc" "$FileInDst"
                else
                    cp -rfp $FileInSrc $FileInDst
                fi
            else
                if [[ !(-e "$FileInDst") || (-d "$FileInDst") ]]
                then
                    cp -fp $FileInSrc $FileInDst
                elif [[ $FileInSrc -ot $FileInDst ]]
                then
                    cp -fp $FileInDst $FileInSrc
                fi
            fi
        done
}

ScanDst $1 $2
ScanSrc $1 $2
echo "TEST"