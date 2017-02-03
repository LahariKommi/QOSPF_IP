  #!/bin/bash  
 sudo apt-get install vsftpd
 echo 'write_enable=YES' >> /etc/vsftpd.conf
 echo 'local_umask=022' >> /etc/vsftpd.conf
 echo 'chroot_local_user=YES' >> /etc/vsftpd.conf
 echo 'allow_writeable_chroot=YES' >> /etc/vsftpd.conf
 echo 'pasv_enable=Yes' >> /etc/vsftpd.conf
 echo 'pasv_min_port=40000' >> /etc/vsftpd.conf
 echo 'pasv_max_port=40100' >> /etc/vsftpd.conf
 sudo service vsftpd restart
 sudo useradd -m john -s /usr/sbin/nologin
 echo "john" |passwd john --stdin
 echo '/usr/sbin/nologin' >> /etc/shells