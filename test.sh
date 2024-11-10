test_dir="assign2-tests/"
tests=("test00" "test01" "test02" "test03" "test04" "test05" "test06" "test07" 
"test08" "test09" "test10" "test11" "test12" "test13" "test14" "test15" 
"test16" "test17" "test18" "test19")
results=("10 : plus" "22 : plus" "24 : plus, minus" "27 : plus, minus" 
"10 : plus, minus
26 : foo
33 : foo" 
"33 : plus, minus"
"10 : plus, minus
26 : clever"
"10 : plus, minus
28 : clever
30 : clever"
"10 : plus, minus
26: clever"
"10 : plus, minus
14 : foo 
30 : clever"
"15 : plus, minus
19 : foo
35 : clever"
"15 : foo
16 : plus 
32 : clever"
"15 : foo
16 : plus, minus 
32 : clever"
"30 : foo, clever
31 : plus, minus"
"24 : foo  
31 : clever,foo
32 : plus"
"14 : plus, minus
24 : foo
27 : foo")

for i in {0..15};
do
    test=${tests[$i]}
    expected=${results[$i]}
    test_path="${test_dir}${test}.c"
    bc_path="${test_dir}${test}.bc"
    clang -emit-llvm -c -O0 -g3 $test_path -o $bc_path
    ./build/llvmassignment $bc_path 2>tmp.txt
    res=$(cat tmp.txt)
    # echo $res
    # echo $expected
    if [ "$expected" = "$res" ];then
        echo "* success $test"
    else 
        echo "x failed $test"
        break
    fi
done

rm tmp.txt
