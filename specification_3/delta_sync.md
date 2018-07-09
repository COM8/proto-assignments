## Delta Sync
In order to do file sync we check periodically if there were files changed (It's planned to switch to Filsystem watcher to monitor for changes). To detect which files were changed we compare the [MD5's](https://en.wikipedia.org/wiki/Md5) of the files to internally saved hashes of them on the last run.

* _If the files weren't there in the prior_ run, they get marked for complete transmission.
* _If the hash changed_ we chunk the data into 900 Byte blocks. For each block we will calculate the [CRC32](https://en.wikipedia.org/wiki/Cyclic_redundancy_check#CRC-32_algorithm). Afterward's it all CRC32 will get compared to the corresponding CRC32 of the previous run. If a CRC32 changed, it replaces the old CRC32 and the Part get's marked for transmission.
* _If nothing changed_, nothing will be send.