#!/bin/bash
 #
 # This file is part of rasdaman community.
 #
 # Rasdaman community is free software: you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation, either version 3 of the License, or
 # (at your option) any later version.
 #
 # Rasdaman community is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 # See the GNU  General Public License for more details.
 #
 # You should have received a copy of the GNU  General Public License
 # along with rasdaman community.  If not, see <http://www.gnu.org/licenses/>.
 #
 # Copyright 2003 - 2016 Peter Baumann / rasdaman GmbH.
 #
 # For more information please see <http://www.rasdaman.org>
 # or contact Peter Baumann via <baumann@rasdaman.com>.
 #
# OVERVIEW:
# Runs the tools that are need to generate the parser from the wcps.g4 grammar
#
# USAGE:
#  cd $PETASCOPE_SOURCES/wcps/parser && ./buildParser.sh
#

URL_TO_ANTLR="http://www.antlr.org/download/antlr-4.1-complete.jar"
PATH_TO_ANTLR_TOOL="/tmp/antlr-4.1-complete.jar"
if [ ! -f "$PATH_TO_ANTLR_TOOL" ]; then
    echo "downloading antlr-4.1 to $PATH_TO_ANTLR_TOOL"
    wget "$URL_TO_ANTLR" -O "$PATH_TO_ANTLR_TOOL"
fi
echo "building parser..."

#Backup the visitor classes that define the actions to be taken on parsing
mkdir -p backupEvaluator
mv WcpsEvaluator.java backupEvaluator
mv WcpsTranslator.java backupEvaluator

#Clean any existing files from the existent parser
rm -f *.java

#Build the parser
export antlr4="java -jar $PATH_TO_ANTLR_TOOL"
export CLASSPATH=".:$PATH_TO_ANTLR_TOOL:$CLASSPATH"
$antlr4 -package petascope.wcps.parser wcps.g4
$antlr4 -package petascope.wcps.parser -no-listener -visitor wcps.g4

#Compile the parser classes
javac *.java

#Put back the visitor classes into the folder
mv backupEvaluator/WcpsEvaluator.java .
mv backupEvaluator/WcpsTranslator.java .

#Cleanup
rm -rf backupEvaluator
rm -f *.class
rm -f *.tokens

echo "done."
