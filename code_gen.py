import os
import re 
import json

class System:
    def __init__ (self, processors, sampling_period, n_tasks, n_task_sets, tasks, tasks_processor_wise):
        self.processors = processors
        self.sampling_period = sampling_period
        self.n_tasks = n_tasks
        self.n_task_sets = n_task_sets
        self.tasks = tasks
        self.tasks_processor_wise = tasks_processor_wise
        self.task_handles = []
        self.queue_handles = []

class Task:
    def __init__(self, name, processor, execution_time, period, task_type):
        self.name = name
        self.processor = processor
        self.execution_time = execution_time
        self.period = period
        self.task_type = task_type

def extract_number(string):
    match = re.search(r'\d+$', string)
    if match:
        return int(match.group())
    else:
        return None

def parse_json_file(filename):
    with open(filename, 'r') as file:
        data = json.load(file)
    return data

def read_config():
    parsed_data = parse_json_file("config.json")
    processors = parsed_data["processors"]
    sampling_period = parsed_data["sampling_period"]
    n_tasks = parsed_data["n_tasks"]
    n_task_sets = parsed_data["n_task_sets"]
    tasks_dict = parsed_data["tasks"]
    tasks_processor_wise={}
    for processor in range(processors):
        tasks_processor_wise[processor+1] = []

    tasks = []
    for key, task in tasks_dict.items():
        tasks.append(Task(task["name"], task["processor"], task["execution"], task["period"], task["type"]))
        for processor in range(processors):
            if task["processor"] == processor+1:
                tasks_processor_wise[processor+1].append(task)
    # print(tasks_processor_wise)
    our_system = System(processors, sampling_period, n_tasks, n_task_sets, tasks, tasks_processor_wise)
    return our_system

def include_headers():
    header_file = open("assets/includes.txt", "r")
    data = header_file.read() + "\n\n"
    return data

def include_timing_config(system, processor):
    tasks = system.tasks_processor_wise[processor]
    output = "//Timing configuration\n"
    for task in tasks:
        task_name = task["name"]
        execution = task["execution"]
        output += f"TickType_t xTask{task_name}_duration = {execution};\n"

    return output + "\n\n"

def include_queue_handles(system, processor):
    tasks = system.tasks_processor_wise[processor]
    output = "//Queues for input and output messages\n"
    for task in tasks:
        task_name = task["name"]
        execution = task["execution"]
        task_set = extract_number(task_name)
        queue_handle=""
        if task["type"]=="S":
            queue_handle = f"xQueue_{task_name}_mx{task_set}"
            system.queue_handles.append(queue_handle)
            output += f"QueueHandle_t {queue_handle};\n"
        elif task["type"]=="A":
            queue_handle = f"xQueue_{task_name}_my{task_set}"
            system.queue_handles.append(queue_handle)
            output += f"QueueHandle_t {queue_handle};\n"
        elif task["type"]=="P":
            queue_handle = f"xQueue_{task_name}_mx{task_set}"
            system.queue_handles.append(queue_handle)
            output += f"QueueHandle_t {queue_handle};\n"

            queue_handle = f"xQueue_{task_name}_my{task_set}"
            system.queue_handles.append(queue_handle)
            output += f"QueueHandle_t {queue_handle};\n"
    return system, output+"\n\n"

def include_task_handles(system, processor):
    tasks = system.tasks_processor_wise[processor]
    output = "//Task handles\n"
    for task in tasks:
        task_name = task["name"]
        task_handle = f"xTask{task_name}_handle"
        system.task_handles.append(task_handle)
        output += f"TaskHandle_t {task_handle};\n"
    output+="TaskHandle_t xTaskControl_handle;\n"

    return system, output+"\n\n"

def include_message_struct(system, processor):
    tasks = system.tasks
    output = "//Message struct\n"
    output += "typedef enum {\n"
    message_enum_list=[]
    for task in tasks:
        task_set = extract_number(task.name)
        if task.task_type=="S":
            message_enum_list.append(f"    emx{task_set}")
        elif task.task_type=="A":
            message_enum_list.append(f"    emy{task_set}")
        elif task.task_type=="S":
            message_enum_list.append(f"    emx{task_set}")
            message_enum_list.append(f"    emy{task_set}")
    
    for message_type in message_enum_list:
        if message_type == message_enum_list[-1]:
            output += f"{message_type}\n" 
        else:
            output += f"{message_type},\n" 
    
    output += "} DataType_t;\n\n"
    output += "typedef struct {\n    uint8_t ucValue;\n    DataType_t eDataType;\n} Data_t;\n\n"

    return output

def read_task_code(task):
    # Extract task info 
    task_set = extract_number(task["name"])
    code_file = []
    if task["type"] == "S":
        code_file = open("assets/sensing.txt", 'r')
    elif task["type"] == "A":
        code_file = open("assets/actuation.txt", 'r')
    elif task["type"] == "P":
        code_file = open("assets/processing.txt", 'r')

    code = code_file.read()
    code = code.replace("{TASK_NAME}", task["name"])
    code = code.replace("{PROCESSOR}", str(task["processor"]))
    code = code.replace("{TASK_SET}", str(task_set))
    
    return code

def find_task_name(task_handle):
    start_index = task_handle.find("xTask") + len("xTask")
    end_index = task_handle.find("_handle")

    # Extract the substring between "xTask" and "_handle"
    extracted_string = task_handle[start_index:end_index]

    return extracted_string

def include_app_main(system, processor):
    app_main = open("assets/app_main.txt", "r").read()

    queue_create = ""
    task_create = ""

    for queue_handle in system.queue_handles:
        queue_create +=f"    {queue_handle} = xQueueCreate(5, sizeof(Data_t));\n"

    for task_handle in system.task_handles:
        task_name = find_task_name(task_handle)
        task_create += f"    xTaskCreatePinnedToCore(vTask_{task_name}, \"{task_name}\", 2048, (void *) NULL, 2, &{task_handle}, 1);\n"
    task_create+=f"    xTaskCreatePinnedToCore(vTaskControl, \"Control Task\", 4096, (void *) NULL, 3, &xTaskControl_handle, 1);\n"

    app_main = app_main.replace("{QUEUE_CREATE}", queue_create)
    app_main = app_main.replace("{TASK_CREATE}", task_create)

    return app_main

def gen_code_processor_wise(system):
    n_processors = system.processors
    for processor in range(n_processors):
        file = open(f"processor_{processor+1}.c", "w")
        #Clean task_handles and queue_handles variables
        system.task_handles = []
        system.queue_handles = []
        
        headers = include_headers()
        timing_config = include_timing_config(system, processor+1)
        system, queue_output = include_queue_handles(system, processor+1)
        system, task_handles_output = include_task_handles(system, processor+1)

        #Include message struct
        message_struct_output = include_message_struct(system, processor+1)

        #Include task codes
        task_code = ""
        for task in system.tasks_processor_wise[processor+1]:
            task_code += read_task_code(task) + "\n\n"
        
        #Include app_main()
        app_main = include_app_main(system, processor)

        file.write(headers)
        file.write(timing_config)
        file.write(queue_output)
        file.write(task_handles_output)
        file.write(message_struct_output)
        file.write(task_code)
        file.write(app_main)

        file.close()


def main():
    system = read_config()
    gen_code_processor_wise(system)

if __name__ == "__main__":
    main()
