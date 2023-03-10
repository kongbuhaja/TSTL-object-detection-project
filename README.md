# [최종제출] 객체인식 모의 경진대회 YouOnlyLiveOnce팀
## - YOLO기반 표지판 검출 및 차선인식 주행 프로젝트  

## 프로젝트 설명
- 차선 유지 주행
- 갈림길 road sign(left or right) 판단 후 주행
- 정지선 + stop, crosswalk sign 시 5초 정지 후 출발
- 정지선 + red light 에서 정지 후 green light 변경 시 출발

## 팀원 역할 담당
- 안현석: YOLO 학습, 데이터 수집, Labeling
- 진창용: 데이터 수집, Labeling, 모델 변환
- 형승호: Hough transform기반 차선인식, 인식된 표지판을 사용하여 주행 전략 및 주행 제어 구현
- 장명근: Hough circle을 사용한 신호등 색 인식 

## 개발 환경
- xycar: Jetson tx2 보드 사용
- training: AWS server, 개인 PC
- 코딩 언어
  - YOLO: Python, Pytorch
  - 차선 인식, 주행 제어: ROS, C++

## YOLO 기반 표지판 검출
### YOLO
- Jetson tx2 보드 사용으로 YOLOv3-tiny 모델 사용
- server, pc에서 학습후 xycar에 모델 탑재
- pth file -> onnx -> trt
- 강의에서 제공되는 가이드라인 코드 기반 학습
- Darknet에서 제공되는 weight기반 전이학습

### 데이터셋
- Labeling class: Left, Right, Stop, Crosswalk, ~~Uturn~~, Traffic_light  
- train data objects: [767, 583, 271, 284, ~~23~~, 268]  
- eval data objects: [79, 46, 42, 75, ~~0~~, 62]   
- 알아볼 수 없거나, 특정 크기 이하의 object는 masking처리하여 사용  
- Uturn은 오검출을 방지하기위해 학습하였으며, 실제 환경에서는 미사용
- Left sign  
![left](https://user-images.githubusercontent.com/42567320/215160123-76c039d4-3ebb-41cb-a5da-c167fa74ff71.png)

- Right sign  
![right](https://user-images.githubusercontent.com/42567320/215160139-0e901690-72b8-41a5-8b06-e89b47acccc4.png)

- Stop sign  
![stop](https://user-images.githubusercontent.com/42567320/215160166-0289b56f-b245-4d65-a0d2-ade6162c8e47.png)

- Crosswalk sign  
![crosswalk](https://user-images.githubusercontent.com/42567320/215160219-b6f91f9b-95b8-4e37-b521-284584e2547c.png)

- ~~Uturn sign~~  
![uturn](https://user-images.githubusercontent.com/42567320/215160241-20824526-6971-417d-9cfb-fc4af4080829.png)

- Traffic light  
![trafic](https://user-images.githubusercontent.com/42567320/215160258-60707c09-f568-4c56-a6a9-d4e75a14a182.png)


### 데이터 수집 방법
- xycar에 장착된 카메라(ELP 2MP OV2710 CMOS) 사용
- 640 * 480 크기의 이미지를 학습에 사용
- "https://github.com/developer0hye/Yolo_Label" 을 사용하여 라벨링 

### 사용한 어그멘테이션  
- imgaug 라이브러리 사용
  - Sharpen: 0.0 ~ 0.1  
  - Affine: translate(-0.1 ~ 0.1), rotate(-3 ~ 3), scale(1.0, 1.5)  
  - Trans_brightness: 0.8 ~ 0.95  
  - Horizontal_flip  

### 추가 적용
- masking: 데이터중 필요없거나, 특정 크기 이하의 object masking  
- make_all: image, annotation file 확인 후 이상없거나, object존재 시 all.txt에 리스트로 작성   
- count_object: dataset중 object별 전체 수를 count  
- kmeans: masking 처리후 object의 크기를 기반으로 kmeans clustering기법을 통해 anchor 추출  
- image_show: labeling 결과 확인  


### 학습 결과
![evaluation](https://user-images.githubusercontent.com/42567320/215160737-7ac445b2-d397-4769-8892-e00396903fb9.png)
![tensorboard](https://user-images.githubusercontent.com/42567320/215160297-05a0b4e5-a69b-4125-8534-ca884e95c5f3.png)

## 제어 알고리즘
- ROS Node

 ![ROS Node image](https://github.com/prgrms-ad-devcourse/ad-4-object-detection-project/blob/YouOnlyLiveOnce/Xycar_Control/Result%20Image/ROS%20Node.PNG)
- 제어 전략
  - Yolo 기반 표지판 검출과 Hough 기반 차전 검출을 통해 xycar의 회전각을 제어 (속도는 일정하게 유지)
  - 제어 전략은 크게 3가지 중요한 부분을 생각하여 설계함
  
1. 갈림길에서 우회전, 좌회전 표지판을 인식하여 주행
  - 표지판 인식 결과를 통해 우회전과 죄회전을 판단.
  - 우회전에서는 오른쪽 차선을 통해 왼쪽 차선의 위치를 추정. (우회전시 왼쪽 차선이 안보이는 경우가 있기 때문)
  - 좌회전에선는 왼쪽 차선을 통해 오른쪽 차선의 위치를 추정.
2. Hough 기반 차선 검출이 실패하였을 때의 주행
  - 양쪽 차선이 존재하지 않아, 차선 검출이 실패했을 경우 -> 직진 (차선이 없는 구간은 교차로 구간이기 때문)
  - 한쪽 차선이 카메라 검출되지 않는 경우 -> 이전 프레임의 차선 위치를 사용
  - 이전 프레임과 비교하여 너무 멀리 떨어진 차선을 검출하는 경우 -> 이전 프레임의 차선 위치를 사용.
3. 정지선을 인식하고, 멈추는 동작
  - 카메라에서 정지선이 보이는 위치에 ROI를 설정하고, Hough 기법을 통해 수평선을 검출
  - 검출된 수평선이 일정 이상 개수 검출된다면 정지선이 있다고 판단
  - 정지선과 정지 혹은 횡단보도 표지판이 검출된다면 정지



## 신호등 색 분류 방법
### 신호등 분류 알고리즘
- 검출된 신호등 ROI의 1/4, 2/4, 3/4 지점의 column에서의 RGB 값을 사용하여 신호등 색 분류
- 각 column에서 (R - G) 값을 모두 더해서 양수라면, 빨간불, 음수라면 초록불로 분류
  - 위와 같이 할 수 있는 이유는 불이 켜진 곳의 R, G의 값은 모두 크며, 불이 꺼진 곳은 R 혹은 G 값만 크기 때문

### 신호등 분류 알고리즘 (시행착오)
- 검출된 신호등 ROI를 canny를 통해 외곽선을 검출한 후, hough circle을 통해 원검출 후 해당 원 내부의 색 판단  
  -> Hough circle을 사용한 원 검출이 부정확함.
- 검출된 신호등 ROI를 밝기 값을 기준으로 threshold 값 이상만 masking한 후, 해당 픽셀의 위치를 평균내어 검출한 원의 RGB값 분석으로 색상판단  
  -> 요구 정확도는 나오나, 채택한 알고리즘 대비 연산량이 많아 탈락

### 최종결과
 - Yolo 인식 결과를 네모 박스와 함께 출력하였으며, 신호등은 신호등 색에 맞게 빨강, 초록으로 구분하였음.
 - 파란 선은 왼쪽 차선을 인식한 결과이며, 초록 선은 오른쪽 차선을 인식한 결과임.
 - 검은색 네모 박스는 이미지의 중심, 빨간 네모 박스는 조향각이 크기를 의미함.
 - 좌회전 표지판을 인식하여 갈림길에서 좌회전하는 장면
 
 ![Left Turn](https://github.com/prgrms-ad-devcourse/ad-4-object-detection-project/blob/YouOnlyLiveOnce/Xycar_Control/Result%20Image/Left%20turn.gif)
 
 - 정지 표지판을 인식하여 정지선에서 멈추는 장면
 
 ![Stop sign](https://github.com/prgrms-ad-devcourse/ad-4-object-detection-project/blob/YouOnlyLiveOnce/Xycar_Control/Result%20Image/Stop%20Sign.gif)
 
 - 신호등을 인식하고, 빨간불에서는 정지하고 초록불에서 출발하는 장면
 
 ![Traffic Sign](https://github.com/prgrms-ad-devcourse/ad-4-object-detection-project/blob/YouOnlyLiveOnce/Xycar_Control/Result%20Image/Traffic%20light.gif)
