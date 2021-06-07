# uselessfs

### to build:

make

### to run in background:

sh setup_bak.sh

创建example目录并将其用uselessfs管理，后台运行

sh shutdown_bak.sh

取消uselessfs的管理，删除example目录

### to run in frontground:

sh setup_f.sh

创建example目录并将其用uselessfs管理，前台运行

此状态可以双终端调试，uselessfs终端输出代码中的debug信息，操作终端进行文件操作


### 可用命令:

mkdir xxx（创建新目录）

rm -r xxx（删除目录）

touch xxx（创建新文件）

rm xxx（删除文件）

echo "xxx" >> xxx（写文件，只有>>可以，预计是没实现清空文件）

cat xxx（读文件，输出全部内容）

ls（查看当前目录文件）