create database if not exists order_system charset=utf8 collate=uft8_bin;

use order_system;

create table if not exists dish_table(
    dish_id int not null primary key auto_increment,
    name varchar(50),
    price int  
);

insert into dish_table values(null, "红烧肉", 2000);
insert into dish_table values(null, "回锅肉", 2000);
insert into dish_table values(null, "玉米鸡丁", 2000);
insert into dish_table values(null, "豆角茄子", 2000);
insert into dish_table values(null, "酸辣土豆丝", 2000);

create table if not exists order_table(
    order_id int not null primary key auto_increment, 
    table_id varchar(50),
    time varchar(50),
    dishes varchar(1024),
    status int
);

insert into order_table values(null, "青龙", "2019-08-03 12:00", "1", 0);
insert into order_table values(null, "白虎", "2019-08-03 12:00", "1,4", 0);
insert into order_table values(null, "朱雀", "2019-08-03 12:00", "2,4", 0);
insert into order_table values(null, "玄武", "2019-08-03 12:00", "2,3,4", 0);
