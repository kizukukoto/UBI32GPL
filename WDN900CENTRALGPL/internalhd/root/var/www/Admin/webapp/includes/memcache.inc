<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.

$memcache = new Memcache;
$memcache->connect('localhost', 11211) or die ("failed to connect to memcached on localhost:11211, is it running?");

$version = $memcache->getVersion();
echo "Memcached version: ".$version."<br/>\n";


?>