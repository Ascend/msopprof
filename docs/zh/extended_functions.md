# **扩展功能**

## json配置文件说明

编写算子的定义json文件，配置参数的具体说明请参考[**表 1**  json文件配置参数说明](#json文件配置参数说明)和[**表 2**  test_case参数字段说明](#test_case参数字段说明)。

例如，json配置文件的命名为add\_test.json，开发者可基于该模板修改测试数据及其他配置参数。

```json
{
  "kernel_name": "add_custom",
  "kernel_path": "./add_custom.o",
  "blockdim": 8,
  "mode": "ca",
  "device_id": 0,
  "magic": "RT_DEV_BINARY_MAGIC_ELF_AIVEC",
  "test_cases": [
    {
      "case_name": "Test_AddCustom_001",
      "param_desc": [
        {
          "param_type": "input",
          "type": "float16",
          "shape": [
            8,
            2048
          ],
          "data_path": "./input_x.bin",
          "name": "x"
        },
        {
          "param_type": "input",
          "type": "float16",
          "shape": [
            8,
            2048
          ],
          "data_path": "./input_y.bin",
          "name": "y"
        },
        {
          "param_type": "output",
          "type": "float16",
          "shape": [
            8,
            2048
          ],
          "name": "z"
        },
        {
          "param_type": "workspace",
          "user_workspace_size": 4096
        },
        {
          "param_type": "tiling",
          "tiling_data_size": 8,
          "tiling_data_path": "./tiling.bin"
        }
      ]
    }
  ]
}
```

**表 1**  json文件配置参数说明<a id="json文件配置参数说明"></a>

|参数名称|参数描述|类型|是否必选|
|---|---|---|---|
|kernel_name|核函数名称。|string|是|
|kernel_path|核函数二进制.o文件所在路径，可配置为绝对路径或者相对路径。|string|是|
|blockdim|核函数运行所需的核数，默认值：1。|int|否|
|mode|测试模式。<br> - 上板：onboard <br> - 性能仿真：ca|string|是|
|device_id|运行时使用AI处理器的ID，默认值：0。|int|否|
|tiling_key|当前动态算子的tiling key。|uint64|否|
|magic|算子类型。<br> - Cube算子：RT_DEV_BINARY_MAGIC_ELF_AICUBE <br> - Vector算子：RT_DEV_BINARY_MAGIC_ELF_AIVEC <br> - Mix融合算子：RT_DEV_BINARY_MAGIC_ELF（仅<term>Atlas A3 训练系列产品/Atlas A3 推理系列产品</term>和<term>Atlas A2 训练系列产品/Atlas A2 推理系列产品</term>支持配置）|string|是|
|test_cases|测试数据，支持列表，每个元素包含一个用例。详细说明可参考[**表 2**  test_case参数字段说明](#test_case参数字段说明)。|map|是|

> [!NOTE] 说明
> 
> - tiling\_key参数仅适用于动态算子。
> - 在使用magic参数时，<term>Atlas 推理系列产品</term>需配置为RT\_DEV\_BINARY\_MAGIC\_ELF。
> - 在使用test\_cases参数时，算子上板或仿真调优时仅支持配置单个用例。

**表 2**  test\_case参数字段说明<a id="test_case参数字段说明"></a>

<table><thead align="left"><tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row1958882429"><th class="cellrowborder" colspan="3" valign="top" id="mcps1.2.7.1.1"><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1859148144216">参数</p>
</th>
<th class="cellrowborder" valign="top" id="mcps1.2.7.1.2"><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p17594894219">说明</p>
</th>
<th class="cellrowborder" valign="top" id="mcps1.2.7.1.3"><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p183566631519">类型</p>
</th>
<th class="cellrowborder" valign="top" id="mcps1.2.7.1.4"><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p766081315143">是否必选</p>
</th>
</tr>
</thead>
<tbody><tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row7593813426"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p420842165820">case_name</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1620814217584">-</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p621330155713">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p620812225819">测试用例的名称，需唯一。</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1356126191512">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p12660213201412">是</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row228167105811"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p161591620135916">param_desc</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p3159520185913">-</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p770893013570">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p4159162075916">用例描述，支持列表，每个元素代表一个核函数参数。</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p335610614152">list</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p8625748205216">是</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row395535810590"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p7340135810572">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1395665819595">param_type</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1277910895615">input/output/workspace/tiling/fftsAddr</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p209523339431">参数类型。</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p113566611518">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p77111049195217">是</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row14519172175812"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p16567163510586">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1751912215589">type</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p3526536165810">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p6519221125814">输入输出数据支持的数据类型，例如：uint8、int16、int32、float16、float32、float等。</p>
<p id="p157634518485">当<span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue139101824171913">“param_type”</span>为input、output时必选。</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p153569610156">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p866017132146">否</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row16288640125810"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p10773183215215">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p12881405585">shape</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p15823211044">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p16288040175819">输入输出Tensor支持的形状，所有输入输出Tensor需支持相同数量的形状。</p>
<p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_zh-cn_topic_0000001214862872_p5132810134916">例如：[8, 3, 256, 256]。</p>
<p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_zh-cn_topic_0000001214862872_p1213211102498">若输入非法的形状会报错，例如：[0]。</p>
<p id="p94333064915">当<span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue10294141615195">“param_type”</span>为input、output时必选。</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p10356176101520">list</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p76603135141">否</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row52426509594"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p17778832326">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p5242050165910">data_path</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p6828121843">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p324215055915">输入数据bin文件的路径。</p>
<ul id="ul1083781334916"><li>当<span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue47675439188">“param_type”</span>为input时必须输入data_path或value_range，且data_path优先级更高。</li><li>若json文件的"data_path"字段为空，需将json文件中设置为"data_path":"null"。json文件具体内容请参见<a href="#json配置文件说明">json配置文件说明</a>。</li></ul>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p135676171515">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p666081310140">否</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row1825765355911"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p8783832225">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p14258053165914">name</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p13833911544">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p7258175335917">参数名称，需唯一。</p>
<p id="p11342112324910">当<span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue1010223718184">“param_type”</span>为input、output时必选。</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p935666141513">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p16601513151410">否</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row13953193217319"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p07531721646">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p18953732531">user_workspace_size</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p77378388413">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p543694745013">用户设置的workspace_size大小。</p>
<p id="p196121931154911">当<span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue679093317186">“param_type”</span>为workspace时必选。</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p235676141519">int</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p14660111371415">否</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row184573117510"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p156912346511">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p445711111156">tiling_data_size</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p121521036651">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p137712027185116">tiling数据大小。</p>
<p id="p151211383492">当<span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue13366929171818">“param_type”</span>为tiling时必选。</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p8357136191511">int</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p14660151381412">否</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row831318201955"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p13574734354">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p731318202052">tiling_data_path</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p715717361052">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1145144815519">tiling数据bin文件所在路径。</p>
<p id="p6227174694915">当<span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue18407152416188">“param_type”</span>为tiling时必选。</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p735716681510">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1966018134148">否</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row354884215210"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p618845215212">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1718865275218">data_size</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p9188155215522">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p195481842105215">fftsAddr的data_size大小。</p>
<p id="p195766538497">当<span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue111181527191815">“param_type”</span>为fftsAddr时必选。</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p63579615159">int</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1766051391419">否</p>
</td>
</tr>
</tbody>
</table>

> [!NOTICE] 须知
> 
> - “output”中参数取值的个数都要与“input”一致，否则测试用例生成会失败。
>    例如：“input”的type支持的类型个数为2，则“output”的type支持的类型个数也需要为2。
>    同理，所有input和output中的type、shape和value\_range的取值个数也需要保持一致。
> - 一个算子所有“input”中参数取值的个数都要一致，否则测试用例生成会失败。
>    所有“input”中的type、shape和value\_range的取值个数也需要保持一致。

## mstx扩展功能

**mstx接口简介**

mstx接口是MindStudio提供的一个性能分析接口，它允许用户在应用程序中插入特定的标记，以便在性能分析时能够更精确地定位关键代码区域，具体接口明细请参见[**表 1**  C/C++ mstx接口列表](#C-mstx接口列表)和[**表 2**  Python mstx接口列表](#Python-mstx接口列表)。具体接口的使用情况请参考《[MindStudio mstx API参考](https://www.hiascend.com/document/detail/zh/mindstudio/82RC1/API/mstxAPIReference/msprof_tx_0001.html)》。

**表 1**  C/C++ mstx接口列表<a id="C-mstx接口列表"></a>

|接口名称|接口说明|msOpProf工具支持情况|
|---|---|---|
|mstxRangeStartA|mstx range指定范围能力的起始位置标记。|支持|
|mstxRangeEnd|mstx range指定范围能力的结束位置标记。|支持|

**表 2**  Python mstx接口列表<a id="Python-mstx接口列表"></a>

|接口名称|接口说明|msOpProf工具支持情况|
|---|---|---|
|mstx.range_start|mstx range指定范围能力的起始位置标记。|支持|
|mstx.range_end|mstx range指定范围能力的结束位置标记。|支持|

**mstx接口的使用**

- msOpProf工具允许用户通过mstx接口实现特定算子调优的功能，使用mstx接口可以自定义采集代码段范围内或指定关键函数的开始和结束时间点，并识别关键函数或计算API等信息，对性能问题快速定界。
- 默认情况下mstx接口不使能。若用户在应用程序中调用mstx接口，工具会根据具体使用场景使能mstx打点功能。例如配置--mstx=on使能用户程序中的mstx API，并可以通过--mstx-include使能用户程序中特定的mstx API，具体使用可分别参见msopprof模式用户指南的“[命令参考](./msopprof_user_guide.md#命令参考)”和msopprof simulator模式用户指南的“[命令参考](./msopprof_simulator_user_guide.md#命令参考)”中的--mstx和--mstx-include参数。
- mstx当前提供了两种API的使用方式：库文件和头文件，以[链接](https://gitee.com/ascend/samples/tree/master/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation)为例：

    > [!NOTE] 说明
    > 
    > - 此样例工程不支持<term>Atlas A3 训练系列产品</term>。
    > - $\{INSTALL\_DIR\}请替换为CANN软件安装后文件存储路径。以root用户安装为例，安装后文件默认存储路径为：/usr/local/Ascend/cann。

    - 在\$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/AclNNInvocation/src/CMakeLists.txt路径下新增库文件libms\_tools\_ext.so，地址为：$\{INSTALL\_DIR\}/lib64/libms\_tools\_ext.so。

        ```shell
        # Header path
        include_directories(
             ...
            ${CUST_PKG_PATH}/include
        )
        ...
        target_link_libraries( 
            ...
            dl
        )
        ```

    - 在\$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/AclNNInvocation/src/main.cpp路径下，将用户程序编译链接dl库，对应的头文件ms\_tools\_ext.h地址：$\{INSTALL\_DIR\}/include/mstx。

        ```shell
        ...
        #include "mstx/ms_tools_ext.h"
        ...
        ```

**调用示例**

启动msOpProf工具后，执行```msprof op --mstx=on --mstx-include=range1 --launch-count=2 python cal.py```命令，将会采集range1范围内的算子，即sub和mul算子。

```python
import mstx
import torch
import torch_npu
x = torch.Tensor([1,2,3,4]).npu()
y = torch.Tensor([1,2,3,4]).npu()
a = x + y
range1_id = mstx.range_start("range1", None)
b = a - x
c = a * x
mstx.range_end(range1_id)
f = x + e
range2_id = mstx.range_start("range2", None)
e = torch.abs(y)
mstx.range_end(range2_id)
```
